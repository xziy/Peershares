#include "distribution.h"

void DividendDistributor::Distribute(double nDistributedAmount, double nMinimumPayout)
{
    BalanceMap mapRetainedBalance(mapBalance);
    bool bMustRedistribute = true;

    while (bMustRedistribute)
    {
        bMustRedistribute = false;

        nTotalDistributed = 0;
        vDistribution.clear();

        BalanceMap::iterator it;
        double nTotalBalance = 0;

        it = mapRetainedBalance.begin();
        while (it != mapRetainedBalance.end())
        {
            nTotalBalance += it->second;
            it++;
        }

        it = mapRetainedBalance.begin();
        while (it != mapRetainedBalance.end())
        {
            double nDistributed = it->second * nDistributedAmount / nTotalBalance;
            if (nDistributed < nMinimumPayout)
            {
                mapRetainedBalance.erase(it++);
                bMustRedistribute = true;
                continue;
            }
            Distribution distribution(it->first, it->second, nDistributed);
            vDistribution.push_back(distribution);

            nTotalDistributed += nDistributed;

            it++;
        }
    }
}


