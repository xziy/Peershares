#include "base58.h"
#include "json/json_spirit_value.h"

class Distribution
{
protected:
    CBitcoinAddress addrPeershares;
    double dBalance;

    CPeercoinAddress addrPeercoin;
    double dDividendAmount;

public:
    Distribution(CBitcoinAddress addrPeershares, double dBalance, double dDividendAmount)
        : addrPeershares(addrPeershares), dBalance(dBalance), addrPeercoin(addrPeershares), dDividendAmount(dDividendAmount)
    {
    }

    const CBitcoinAddress GetPeershareAddress() const
    {
        return addrPeershares;
    }

    double GetBalance() const
    {
        return dBalance;
    }

    const CPeercoinAddress GetPeercoinAddress() const
    {
        return addrPeercoin;
    }

    double GetDividendAmount() const
    {
        return dDividendAmount;
    }
};

typedef std::map<const CBitcoinAddress, int64> BalanceMap;
typedef std::vector<Distribution> DistributionVector;

class DividendDistributor
{
protected:
    const BalanceMap& mapBalance;
    double dTotalDistributed;

    DistributionVector vDistribution;

public:
    DividendDistributor(const BalanceMap& mapBalance) : mapBalance(mapBalance), dTotalDistributed(0)
    {
    }

    void Distribute(double dDistributedAmount, double dMinimumPayout);
    void GenerateOutputs(int nTransactions, std::vector<json_spirit::Object> &vTransactionOuts) const;

    const DistributionVector& GetDistributions() const
    {
        return vDistribution;
    }

    int DistributionCount() const
    {
        return vDistribution.size();
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

    double TotalDistributed() const
    {
        return dTotalDistributed;
    }
};
