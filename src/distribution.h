#include "base58.h"

class Distribution
{
protected:
    CBitcoinAddress addrPeershares;
    int64 nBalance;

    CBitcoinAddress addrPeercoin;
    int64 nDividendAmount;

public:
    Distribution(CBitcoinAddress addrPeershares, int64 nBalance, int64 nDividendAmount)
        : addrPeershares(addrPeershares), nBalance(nBalance), addrPeercoin(addrPeershares), nDividendAmount(nDividendAmount)
    {
    }

    const CBitcoinAddress &GetPeershareAddress() const
    {
        return addrPeershares;
    }

    int64 GetBalance() const
    {
        return nBalance;
    }

    const CBitcoinAddress &GetPeercoinAddress() const
    {
        return addrPeercoin;
    }

    int64 GetDividendAmount() const
    {
        return nDividendAmount;
    }
};

typedef std::map<const CBitcoinAddress, int64> BalanceMap;
typedef std::vector<Distribution> DistributionVector;

class DividendDistributor
{
protected:
    const BalanceMap& mapBalance;
    int64 nTotalDistributed;

    DistributionVector vDistribution;

public:
    DividendDistributor(const BalanceMap& mapBalance) : mapBalance(mapBalance), nTotalDistributed(0)
    {
    }

    void Distribute(int64 nDistributedAmount, int64 nMinimumPayout);

    const DistributionVector& GetDistributions() const
    {
        return vDistribution;
    }

    const Distribution& GetDistribution(const CBitcoinAddress& addrPeershare) const
    {
        for (DistributionVector::const_iterator it = vDistribution.begin(); it != vDistribution.end(); ++it)
        {
            if (it->GetPeershareAddress() == addrPeershare)
                return *it;
        }
        throw std::runtime_error("Distribution not found");
    }

    int64 TotalDistributed() const
    {
        return nTotalDistributed;
    }
};
