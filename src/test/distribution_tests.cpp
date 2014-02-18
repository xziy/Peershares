#include <boost/test/unit_test.hpp>

#include "distribution.h"

BOOST_AUTO_TEST_SUITE(distribution_tests)

BOOST_AUTO_TEST_CASE( test_simple_distribution )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 10;
    mapBalance[CBitcoinAddress(2)] = 30;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(1000000, 10000);

    BOOST_CHECK_EQUAL(250000, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount());
    BOOST_CHECK_EQUAL(750000, distributor.GetDistribution(CBitcoinAddress(2)).GetDividendAmount());
    BOOST_CHECK_EQUAL(1000000, distributor.TotalDistributed());
}

BOOST_AUTO_TEST_CASE( test_off_decimal_distribution )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 1;
    mapBalance[CBitcoinAddress(2)] = 1;
    mapBalance[CBitcoinAddress(3)] = 1;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(1000000, 10000);

    BOOST_CHECK_EQUAL(333333, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount());
    BOOST_CHECK_EQUAL(333333, distributor.GetDistribution(CBitcoinAddress(2)).GetDividendAmount());
    BOOST_CHECK_EQUAL(333333, distributor.GetDistribution(CBitcoinAddress(3)).GetDividendAmount());
    BOOST_CHECK_EQUAL(999999, distributor.TotalDistributed());
}

BOOST_AUTO_TEST_CASE( test_not_enough_dividends_to_pay_fee )
{
    BalanceMap mapBalance;

    mapBalance[CBitcoinAddress(1)] = 90;
    mapBalance[CBitcoinAddress(2)] = 10;

    DividendDistributor distributor(mapBalance);
    distributor.Distribute(100000, 10001);

    BOOST_CHECK_EQUAL(100000, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount());
    BOOST_CHECK_EQUAL(1, distributor.GetDistributions().size());
    BOOST_CHECK_EQUAL(100000, distributor.TotalDistributed());


    distributor.Distribute(100000, 10000);

    BOOST_CHECK_EQUAL( 90000, distributor.GetDistribution(CBitcoinAddress(1)).GetDividendAmount());
    BOOST_CHECK_EQUAL( 10000, distributor.GetDistribution(CBitcoinAddress(2)).GetDividendAmount());
    BOOST_CHECK_EQUAL(100000, distributor.TotalDistributed());
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
