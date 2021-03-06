#ifndef UTREEXO_ACCUMULATOR_H
#define UTREEXO_ACCUMULATOR_H

#include <memory>
#include <state.h>
#include <uint256.h>
#include <vector>

/**
 * Accumulator is an abstract class for a dynamic hash based accumulator.
 */
class Accumulator
{
public:
    class Leaf
    {
    public:
        virtual uint256 hash() const = 0;
        virtual bool remember() const = 0;

        virtual ~Leaf() {}
    };

    class BatchProof
    {
    private:
        const std::vector<uint64_t> targets;
        std::vector<uint256> proof;

    public:
        BatchProof(const std::vector<uint64_t> targets, std::vector<uint256> proof)
            : targets(targets), proof(proof) {}

        bool verify(ForestState state, const std::vector<uint256> roots, const std::vector<uint256> targetHashes) const;
        void print();
    };

    Accumulator(ForestState& state) : state(state)
    {
        this->mRoots.reserve(64);
    }

    virtual ~Accumulator() {}
    virtual const BatchProof prove(const std::vector<uint64_t>& targets) const = 0;

    bool verify(const BatchProof& proof);
    void modify(const std::vector<std::shared_ptr<Accumulator::Leaf>>& leaves,
                const std::vector<uint64_t>& targets);
    const std::vector<uint256> roots() const;

    static uint256 parentHash(const uint256& left, const uint256& right);

protected:
    class Node
    {
    protected:
        const ForestState& forestState;

        // Store the parent.
        // This is useful if you want to rehash a path from the bottom up.
        std::shared_ptr<Accumulator::Node> mParent;

        Node(const ForestState& state,
             uint64_t position)
            : forestState(state), mParent(nullptr), position(position) {}

        Node(const ForestState& state,
             uint64_t position,
             std::shared_ptr<Accumulator::Node> parent)
            : forestState(state), mParent(parent), position(position) {}

    public:
        uint64_t position;
        virtual ~Node() {}

        virtual const uint256& hash() const = 0;
        virtual void reHash() = 0;

        virtual std::shared_ptr<Accumulator::Node> parent() const
        {
            return this->mParent;
        }
    };

    ForestState& state;
    std::vector<std::shared_ptr<Node>> mRoots;

    /*
     * Swap two subtrees in the forest.
     * Return the nodes that need to be rehashed.
     */
    virtual std::shared_ptr<Accumulator::Node> swapSubTrees(uint64_t posA, uint64_t posB) = 0;

    /*
     * mergeRoot and newLeaf only have the desired effect if called correctly.
     * newLeaf should be called to allocate a new leaf.
     * After calling newLeaf, mergeRoot should be called for every consecutive least significant bit that is set to 1.
     */

    // Return the result of the latest merge.
    virtual std::shared_ptr<Accumulator::Node> mergeRoot(uint64_t parentPos, uint256 parentHash) = 0;
    virtual std::shared_ptr<Accumulator::Node> newLeaf(uint256 hash) = 0;

    virtual void finalizeRemove(const ForestState nextState) = 0;

    void printRoots(const std::vector<std::shared_ptr<Accumulator::Node>>& roots) const;
    virtual void add(const std::vector<std::shared_ptr<Accumulator::Leaf>>& leaves);
    void remove(const std::vector<uint64_t>& targets);
};

#endif // UTREEXO_ACCUMULATOR_H
