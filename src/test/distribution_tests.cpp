#include <boost/test/unit_test.hpp>

#include "distribution.h"

BOOST_AUTO_TEST_SUITE(distribution_tests)

#define PRECISION 1e-8

BOOST_AUTO_TEST_CASE( test_simple_distribution )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 10;
    mapBalance[CBitcoinAddress(2)] = 30;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(100, 0.01);

    BOOST_CHECK_EQUAL(25.0, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount());
    BOOST_CHECK_EQUAL(75.0, distributor.GetDistribution(CBitcoinAddress(2)).GetDividendAmount());
    BOOST_CHECK_EQUAL(100.0, distributor.TotalDistributed());
}

BOOST_AUTO_TEST_CASE( test_off_decimal_distribution )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 1;
    mapBalance[CBitcoinAddress(2)] = 1;
    mapBalance[CBitcoinAddress(3)] = 1;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(10, 1);

    BOOST_CHECK_CLOSE(3.3333333333, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount(), PRECISION);
    BOOST_CHECK_CLOSE(3.3333333333, distributor.GetDistribution(CBitcoinAddress(2)).GetDividendAmount(), PRECISION);
    BOOST_CHECK_CLOSE(3.3333333333, distributor.GetDistribution(CBitcoinAddress(3)).GetDividendAmount(), PRECISION);
    BOOST_CHECK_CLOSE(10.0, distributor.TotalDistributed(), PRECISION);
}

BOOST_AUTO_TEST_CASE( test_not_enough_dividends_to_pay_fee )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 90;
    mapBalance[CBitcoinAddress(2)] = 10;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(0.1, 0.010001);

    BOOST_CHECK_CLOSE(0.1, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount(), PRECISION);
    BOOST_CHECK_EQUAL(1, distributor.GetDistributions().size());
    BOOST_CHECK_CLOSE(0.1, distributor.TotalDistributed(), PRECISION);


    distributor.Distribute(0.1, 0.01);

    BOOST_CHECK_CLOSE(0.09, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount(), PRECISION);
    BOOST_CHECK_CLOSE(0.01, distributor.GetDistribution(CBitcoinAddress(2)).GetDividendAmount(), PRECISION);
    BOOST_CHECK_CLOSE(0.10, distributor.TotalDistributed(), PRECISION);
}

BOOST_AUTO_TEST_CASE( test_nobody_has_enough_funds )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 1;
    mapBalance[CBitcoinAddress(2)] = 1;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(20000, 10001);

    BOOST_CHECK_EQUAL(0, distributor.GetDistributions().size());

    BOOST_CHECK_EQUAL(0, distributor.TotalDistributed());
}

BOOST_AUTO_TEST_SUITE_END()
