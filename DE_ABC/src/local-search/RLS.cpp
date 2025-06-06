#include "local-search/RLS.h"
#include "Core.h"
#include "Instance.h"
#include "Solution.h"
#include "constructions/NEH.h"

#include <cassert>
#include <cstdint>
#include <stack>
#include <vector>
#include <iostream>

enum class BlockType : std::uint8_t {
    NORMAL,
    ANTI,
    VERT,
    NONE,
};

struct GraphBlock {
    BlockType type;
    size_t idx_first;
    size_t idx_last;
};

namespace {
std::pair<size_t, size_t> get_next_pos(const Instance &instance, const Solution &s, const size_t j, const size_t k) {
    const auto departure = s.departure_times[j][k];
    const auto p = instance.p(s.sequence[j], k);

    if ((j > 0 && k < instance.num_machines() - 1) && departure == s.departure_times[j - 1][k + 1]) {
        return {j - 1, k + 1};
    }

    if (j > 0 && departure == s.departure_times[j - 1][k] + p) {
        return {j - 1, k};
    }

    return {j, k - 1};
}

BlockType get_block_type(const size_t curr_j, const size_t curr_k, const size_t next_j, const size_t next_k) {
    if (curr_j == next_j) {
        return BlockType::VERT;
    }
    if (curr_k == next_k) {
        return BlockType::NORMAL;
    }
    return BlockType::ANTI;
}

void handle_block_change(std::stack<GraphBlock> &blocks, GraphBlock &curr_block, const BlockType next_block_type,
                         const size_t curr_j, const size_t next_j) {
    if (curr_block.type != next_block_type) {
        if (next_block_type == BlockType::VERT) {
            // Remove this job from previous block
            curr_block.idx_first += 1;
        }

        blocks.push(curr_block);

        if (next_block_type == BlockType::VERT) {
            curr_block = {BlockType::VERT, curr_j, curr_j};
        } else if (next_block_type == BlockType::ANTI) {
            curr_block.idx_last = next_j;
        } else { // NORMAL block
            // If the curr_block is an anti-block than the current node has
            // no value on the path and shouldn't belong to the normal block
            if (curr_block.type == BlockType::ANTI) {
                curr_block.idx_last = next_j;
            } else {
                curr_block.idx_last = curr_j;
            }
        }

        curr_block.type = next_block_type;
    }
}

std::vector<std::pair<size_t, size_t>> grabowski_reinsertion(const Instance &instance, const Solution &s,
                                                             const size_t job) {
    std::stack<GraphBlock> blocks;

    size_t curr_j = instance.num_jobs() - 1;
    size_t curr_k = instance.num_machines() - 1;

    GraphBlock curr_block{BlockType::NONE, curr_j, curr_j};
    BlockType target_block_type = BlockType::NONE;

    while (curr_j > 0 || curr_k > 0) {
        const auto [next_j, next_k] = get_next_pos(instance, s, curr_j, curr_k);
        const auto next_block_type = get_block_type(curr_j, curr_k, next_j, next_k);
        const bool is_curr_target = s.sequence[curr_j] == job;

        // We have reached the end of the target normal block, so we break and handle it afterwards
        if (next_block_type != BlockType::NORMAL && target_block_type == BlockType::NORMAL) {
            break;
        }

        // If the target job is at a vert block, the whole range is valid
        if (next_block_type == BlockType::VERT && is_curr_target) {
            return {std::pair<size_t, size_t>(0, instance.num_jobs() - 1)};
        }

        if (is_curr_target) {
            // If the next block is an anti-block and our current block is of another
            // type then we should use the current block type, since the node that is
            // reached after the last diagonal edge does not belong to the anti-block
            if (next_block_type == BlockType::ANTI) {
                target_block_type = curr_block.type;
            } else {
                target_block_type = next_block_type;
            }
        }

        // When the next block is different from the current
        handle_block_change(blocks, curr_block, next_block_type, curr_j, next_j);

        // Updating NORMAL and ANTI blocks
        if (next_block_type != BlockType::VERT) {
            curr_block.idx_first = next_j;
        }

        curr_j = next_j;
        curr_k = next_k;
    }

    const auto &block = blocks.top();

    const std::pair<size_t, size_t> left(0, block.idx_first - 1);
    const std::pair<size_t, size_t> right(block.idx_last + 1, instance.num_jobs() - 1);

    if (block.idx_first == 0) {
        return {right};
    }
    if (block.idx_last == instance.num_jobs() - 1) {
        return {left};
    }

    return {left, right};
}
} // namespace

bool rls_grabowski(Solution &s, const std::vector<size_t> &ref, Instance &instance) {
    bool improved = false;
    size_t j = 0;
    size_t cnt = 0;
    NEH helper(instance);
    core::recalculate_solution(instance, s); // Set departure times matrix
    while (cnt < instance.num_jobs()) {
        j = (j + 1) % instance.num_jobs();

        const size_t job = ref[j];

        const auto ranges = grabowski_reinsertion(instance, s, job);

        size_t og_index = 0;
        for (size_t i = 0; i < s.sequence.size(); i++) {
            if (s.sequence[i] == job) {
                s.sequence.erase(s.sequence.begin() + (long)i);
                og_index = i;
                break;
            }
        }

        const auto [best_index, makespan] = helper.taillard_grabowski_best_ins(s, job, ranges);

        if (makespan < s.cost) {
            s.sequence.insert(s.sequence.begin() + (long)best_index, job);
            cnt = 0;
            s.cost = makespan;
            improved = true;
            core::partial_recalculate_solution(instance, s, std::min(og_index, best_index));
            continue;
        }

        s.sequence.insert(s.sequence.begin() + (long)og_index, job);

        cnt++;
    }

    return improved;
}

bool rls(Solution &s, Instance &instance) {

    bool improved = false;
    size_t cnt = 0;
    NEH helper(instance);
    std::vector<size_t> ref = s.sequence;
    while (cnt < instance.num_jobs()) {
        
        const size_t job = ref[cnt];
        for (size_t i = 0; i < s.sequence.size(); i++) {
            if (s.sequence[i] == job) {
                s.sequence.erase(s.sequence.begin() + (long)i);
                break;
            }
        }

        auto [best_index, makespan] = helper.taillard_best_insertion(s.sequence, job);
        s.sequence.insert(s.sequence.begin() + (long)best_index, job);

        if (makespan < s.cost) {
            s.cost = makespan;
            improved = true;
        }

        cnt++;
    }

    return improved;
}
