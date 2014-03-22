#include "db.h"
#include "walletdb.h"
#include "net.h"
#include "init.h"
#include "util.h"
#include "scanbalance.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#ifndef WIN32
#include <signal.h>
#endif

using namespace std;
using namespace boost;

static void ScanTransactionInputs(CTxDB& txdb, const CTransaction& tx, BalanceMap& mapBalance)
{
    if (tx.IsCoinBase()) return;

    BOOST_FOREACH(const CTxIn& txi, tx.vin)
    {
        CTransaction ti;
        if (!txdb.ReadDiskTx(txi.prevout, ti))
        {
            string s = strprintf("Failed to load transaction %s", txi.ToStringShort().c_str());
            throw runtime_error(s);
        }
        if (txi.prevout.n >= ti.vout.size())
        {
            string s = strprintf("Invalid index in transaction %s", txi.ToStringShort().c_str());
            throw runtime_error(s);
        }

        CTxOut prevOut = ti.vout[txi.prevout.n];
        CBitcoinAddress addr;
        ExtractAddress(prevOut.scriptPubKey, addr);
        if (prevOut.nValue > 0)
        {
            if (mapBalance.count(addr) == 0) {
                string s = strprintf("Missing input address: %s", txi.ToStringShort().c_str());
                throw runtime_error(s);
            }
            if (prevOut.nValue > mapBalance[addr]) {
                string s = strprintf("Input address would have negative balance: %s", txi.ToStringShort().c_str());
                throw runtime_error(s);
            }
            mapBalance[addr] -= prevOut.nValue;
            if (mapBalance[addr] == 0) mapBalance.erase(addr);
        }
    }
}

static void ScanTransactionOutputs(const CTransaction& tx, BalanceMap& mapBalance)
{
    BOOST_FOREACH(const CTxOut& txo, tx.vout)
    {
        if (txo.nValue > 0 && !txo.scriptPubKey.empty())
        {
            CBitcoinAddress addr;
            ExtractAddress(txo.scriptPubKey, addr);
            mapBalance[addr] += txo.nValue;
        }
    }
}

void GetAddressBalances(unsigned int cutoffTime, BalanceMap& mapBalance)
{
    CBlockIndex* pblk0 = pindexGenesisBlock, *pblk1 = pindexBest;
    if (!pblk0) throw runtime_error("No genesis block.");
    if (!pblk1) throw runtime_error("No best block chain.");

    if (cutoffTime>pblk1->nTime)
        throw runtime_error("Cutoff date later than most recent block.");

    CTxDB txdb("r");
    int nBlks = 0;
    while (pblk0 != pblk1)
    {
        if (pblk0->nTime >= cutoffTime) break;

        CBlock block;
        block.ReadFromDisk(pblk0, true);

        BOOST_FOREACH(const CTransaction& tx, block.vtx)
        {
            ScanTransactionInputs(txdb, tx, mapBalance);
            ScanTransactionOutputs(tx, mapBalance);
        }

        pblk0 = pblk0->pnext;
        nBlks++;
    }
}


void CmdLine_GetAddrBalances()
{
    fprintf(stdout, "******************** gettting balances of all addresss\n");
    string ymd = GetArg("-addrbalance", "");
    fprintf(stdout, "specified local cutoff date: %s\n", ymd.c_str());
    if (ymd.length() == 0) return;

    int i = atoi(ymd.c_str());
    tm t;
    memset(&t, 0, sizeof(t));
    t.tm_mday = i % 100;
    i /= 100;
    t.tm_mon = (i % 100) -1;
    i /= 100;
    t.tm_year = i-1900;
    t.tm_isdst = -1;
    time_t cutoffTime = mktime(&t);
    cutoffTime += 24 * 60 * 60; //actual cutoff at beginning of next day

    BalanceMap mapBalance;
    try
    {
        GetAddressBalances((unsigned int)cutoffTime, mapBalance);
        fprintf(stdout, "GetAddressBalances() returned OK\n");
    }
    catch (const std::exception &error)
    {
        fprintf(stdout, "GetAddressBalances() returned error msg: %s\n", error.what());
    }

    fprintf(stdout, "cutoff day inclusive: y=%d m=%d d=%d\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    fprintf(stdout, "actual cutoff epoch=%u, UTC time: %s\n",
            (unsigned int)cutoffTime, asctime(gmtime(&cutoffTime)));
}
