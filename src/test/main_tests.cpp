// Copyright (c) 2014-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "validation/validation.h"
#include "net.h"
#include "unity/signals.h"

#include "test/test_gulden.h"

#include <boost/signals2/signal.hpp>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(main_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(block_subsidy_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(1),        COIN * 170000000);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(250000),   COIN * 1000);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(250001),   COIN * 100);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(1030001),  COIN * 120);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(10880000), COIN * 120);
    BOOST_CHECK_EQUAL(GetBlockSubsidy(10880001), 0);
}

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    CAmount nSum = 0;
    for (int nHeight = 0; nHeight < 10890000; nHeight++) {
        CAmount nSubsidy = GetBlockSubsidy(nHeight);
        nSum += nSubsidy;
        BOOST_CHECK(MoneyRange(nSum));
    }
    BOOST_CHECK_EQUAL(nSum, 168000000000000000LL);
}

bool ReturnFalse() { return false; }
bool ReturnTrue() { return true; }

BOOST_AUTO_TEST_CASE(test_combiner_all)
{
    boost::signals2::signal<bool (), BooleanAndAllReturnValues> Test;
    BOOST_CHECK(Test());
    Test.connect(&ReturnFalse);
    BOOST_CHECK(!Test());
    Test.connect(&ReturnTrue);
    BOOST_CHECK(!Test());
    Test.disconnect(&ReturnFalse);
    BOOST_CHECK(Test());
    Test.disconnect(&ReturnTrue);
    BOOST_CHECK(Test());
}
BOOST_AUTO_TEST_SUITE_END()
