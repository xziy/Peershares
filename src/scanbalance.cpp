// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2013 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "db.h"
#include "walletdb.h"
//#include "bitcoinrpc.h"
#include "net.h"
#include "init.h"
#include "util.h"
//#include "ui_interface.h"
//#include "checkpoints.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#ifndef WIN32
#include <signal.h>
#endif

using namespace std;
using namespace boost;

typedef map<CBitcoinAddress,int64> AddrBalance;

static void ScanTransactionInputs(CTxDB&txdb,const CTransaction&t,AddrBalance&ab)
{
    fprintf(stdout,"TX %s%s\n", t.IsCoinBase()?"CB ":"", t.GetHash().ToString().c_str());

    if (t.IsCoinBase()) return;

    BOOST_FOREACH(const CTxIn& txi, t.vin )
    {
        CTransaction ti;
        if (!txdb.ReadDiskTx(txi.prevout,ti))
        {
            string s=strprintf("When scanning input of transaction:\n"
                               "  %s\n"
                               "Failed to load transaction:\n"
                               "  %s\n",
                               t.GetHash().ToString().c_str(), txi.ToStringShort().c_str());
            throw runtime_error(s);
        }
        if (txi.prevout.n>=ti.vout.size())
        {
            string s=strprintf("When scanning input of transaction:\n"
                               "  %s\n"
                               "Invalid index in:\n"
                               "  %s\n",
                               t.GetHash().ToString().c_str(), txi.ToStringShort().c_str());
            throw runtime_error(s);
        }


        CTxOut prevOut=ti.vout[txi.prevout.n];
        CBitcoinAddress addr;
        ExtractAddress(prevOut.scriptPubKey,addr);
        fprintf(stdout,"  I from=%s amt=%"PRI64d"\n", addr.ToString().c_str(),prevOut.nValue);
        if (prevOut.nValue>0) //0 in genesis block
        {
            if (0==ab.count(addr)) {
                string s=strprintf("When scanning input of transaction:\n"
                                   "  %s\n"
                                   "Encountered missing input address:\n"
                                   "  %s\n",
                                   t.GetHash().ToString().c_str(), txi.ToStringShort().c_str());
                throw runtime_error(s);
            }
            if (prevOut.nValue>ab[addr]) {
                string s=strprintf("When scanning input of transaction:\n"
                                   "  %s\n"
                                   "Input address would have negative balance:\n"
                                   "  %s\n",
                                   t.GetHash().ToString().c_str(), txi.ToStringShort().c_str());
                throw runtime_error(s);
            }
            ab[addr]-=prevOut.nValue;
            if (0==ab[addr]) ab.erase(addr);
        }
    } //BOOST_FOREACH(const CTxIn& txi, t.vin )
}

static void ScanTransactionOutputs(const CTransaction&t,AddrBalance&ab)
{
    BOOST_FOREACH(const CTxOut& txo, t.vout )
    {
        if (txo.nValue>0 && !txo.scriptPubKey.empty())
        {
            CBitcoinAddress addr;
            ExtractAddress(txo.scriptPubKey, addr);
            ab[addr]+=txo.nValue;
            fprintf(stdout,"  O addr=%s val=%"PRI64d"\n", addr.ToString().c_str(),txo.nValue);
        }
        else fprintf(stdout,"  O %s\n",txo.ToStringShort().c_str());
    }
}


/*Returns number of seconds from cutoff date to the best block time at the time of call.
TODO:
    Sync access to pindexGenesisBlock & pindexBest ?
    Build in-memory transaction map instead of using CTxDB, faster but uses more memory.
    Translation ?
*/
static unsigned int GetAddressBalancesInternal(unsigned int cutoffTime, AddrBalance & ab)
{
    CBlockIndex*pblk0=pindexGenesisBlock, *pblk1=pindexBest;
    if (!pblk0) throw runtime_error("No genesis block.");
    if (!pblk1) throw runtime_error("No best block chain.");

    if (cutoffTime>pblk1->nTime) throw runtime_error("Cutoff date later than most recent block.");

    unsigned int nSecsSinceCutoff=pblk1->nTime - cutoffTime;

    //fprintf(stdout,"%d %d %s\n",pblk0->nHeight,pblk0->nTime,pblk0->ToString().c_str());
    //fprintf(stdout,"%d %d %s\n",pblk1->nHeight,pblk1->nTime,pblk1->ToString().c_str());
    int64 time0 = GetTimeMillis();

    CTxDB txdb("r");
    int nBlks=0;
    while (pblk0!=pblk1)
    { //for each block
        if (pblk0->nTime>=cutoffTime) break;
        CBlock b;
        //CBitcoinAddress addr;
        b.ReadFromDisk(pblk0, true);
        //if (b.IsProofOfStake())
        //if (4818==nBlks) {
            fprintf(stdout,"***B %d %u %u %s\n",nBlks,b.nNonce,b.vtx.size(), b.GetHash().ToString().c_str());
            BOOST_FOREACH(const CTransaction& t, b.vtx)
            {
                ScanTransactionInputs(txdb,t,ab);
                ScanTransactionOutputs(t,ab);
            }
        //} //if (4818==nBlks)
        pblk0=pblk0->pnext;
        nBlks++;
        //if (333==nBlks) break; //for debugging
    }
    fprintf(stdout, "scanned %u blocks, %"PRI64d" ms\n", nBlks, GetTimeMillis() - time0);

    return nSecsSinceCutoff;

}


bool GetAddressBalances(unsigned int cutoffTime,unsigned int&nSecsSinceCutoff,AddrBalance&ab,string&errMsg)
{   bool ret=true;
    
    try {
        nSecsSinceCutoff=GetAddressBalancesInternal((unsigned int)cutoffTime,ab);
        fprintf(stdout,"best block secs since cutoff: %u, days since: %.3f\n",
                nSecsSinceCutoff,(float)nSecsSinceCutoff/(24*60*60));
        fprintf(stdout,"addresses with nonzero balance: %u\n",ab.size());

        fprintf(stdout,"*************** address & balance map ******************\n");
        BOOST_FOREACH(const PAIRTYPE(CBitcoinAddress,int64)&a, ab)
        {
            fprintf(stdout,"%s %s\n",a.first.ToString().c_str(),FormatMoney(a.second).c_str());
        }
        errMsg="";
    } catch (std::exception&e)
    {
        //fprintf(stdout,"GetAddressBalances() returned error msg: %s\n", e.what() );
        errMsg=e.what();
        ret=false;
    } catch (...)
    {
        errMsg="Unknown exception when scanning blockchain.";
        ret=false;
    }
    return ret;
}

//called from AppInit2(), check for command line option -addrbalance=yyyymmdd
void CmdLine_GetAddrBalances()
{
    fprintf(stdout,"******************** gettting balances of all addresss\n");
    string ymd=GetArg("-addrbalance","");
    fprintf(stdout,"specified local cutoff date: %s\n",ymd.c_str());
    if (ymd.length()==0) return;

    int i=atoi(ymd.c_str());
    tm t;
    memset(&t,0,sizeof(t));
    t.tm_mday= i % 100; i/=100; //no error checking on these values
    t.tm_mon= (i % 100) -1; i/=100;
    t.tm_year=i-1900;
    t.tm_isdst=-1;
    time_t cutoffTime=mktime(&t);
    cutoffTime+=24*60*60; //actual cutoff at beginning of next day

    unsigned int nSecsSinceCutoff;
    AddrBalance ab;
    string errMsg;
    bool ok=GetAddressBalances((unsigned int)cutoffTime, nSecsSinceCutoff,ab,errMsg);
    
    if (ok) fprintf(stdout,"GetAddressBalances() returned OK\n");
    else fprintf(stdout,"GetAddressBalances() returned error msg: %s\n", errMsg.c_str() );

    fprintf(stdout,"cutoff day inclusive: y=%d m=%d d=%d\n",t.tm_year+1900,t.tm_mon+1,t.tm_mday);
    fprintf(stdout,"actual cutoff epoch=%u, UTC time: %s\n",
            (unsigned int)cutoffTime, asctime(gmtime(&cutoffTime)) );

}

