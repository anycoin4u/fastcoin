// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "auxpow.h"
#include "arith_uint256.h"
#include "chain.h"
#include "fastcoin.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"


typedef int64_t int64;
typedef uint64_t uint64;

// Determine if the for the given block, a min difficulty setting applies
bool AllowMinDifficultyForBlock(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    // check if the chain allows minimum difficulty blocks
    if (!params.fPowAllowMinDifficultyBlocks)
        return false;

    // Fastcoin: Magic number at which reset protocol switches
    // check if we allow minimum difficulty at this block-height
    if (pindexLast->nHeight < 157500)
        return false;

    // Allow for a minimum block time if the elapsed time > 2*nTargetSpacing
    return (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2);
}

unsigned int GetNextWorkRequired_V2(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Fastcoin: Special rules for minimum difficulty blocks with Digishield
    if (AllowDigishieldMinDifficultyForBlock(pindexLast, pblock, params))
    {
        // Special difficulty rule for testnet:
        // If the new block's timestamp is more than 2* nTargetSpacing minutes
        // then allow mining of a min-difficulty block.
        return nProofOfWorkLimit;
    }

    // Only change once per difficulty adjustment interval
    bool fNewDifficultyProtocol = (pindexLast->nHeight >= 11327900);  //digishieldConsensus.nHeightEffective = 11327900
    const int64_t difficultyAdjustmentInterval = fNewDifficultyProtocol
                                                 ? 1
                                                 : params.DifficultyAdjustmentInterval();
    if ((pindexLast->nHeight+1) % difficultyAdjustmentInterval != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Litecoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = difficultyAdjustmentInterval-1;
    if ((pindexLast->nHeight+1) != difficultyAdjustmentInterval)
        blockstogoback = difficultyAdjustmentInterval;

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - blockstogoback;
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateFastcoinNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget && (hash != uint256S("0xa124332a8d96040c081ff7dc3fac3f7555ea279a6378c0f5ee6c9c19945528fc"))   )
        return false;

    return true;
}

unsigned int GetNextWorkRequired_V1(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();
	int64_t nTargetTimespan = params.nPowTargetTimespan;
	int64_t nTargetSpacing = params.nPowTargetSpacing;
    int64_t nInterval = nTargetTimespan / nTargetSpacing;	// 300
	
	
	
    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

//BitBreak BitSend
const int64_t nLongTimeLimit = 60 * 60; // 60 minutes
const int64_t nMDTimeLimit = 60 * 60 * 24 * 90; // 90 days
const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
arith_uint256 bnBitBreak;
bnBitBreak.SetCompact(pindexLast->nBits);
    
// Reduce difficulty if current block generation time has already exceeded maximum time limit.
if ( ( (pindexLast->nHeight+1) > 11324612 ) && ( (pblock->nTime - pindexLast->GetBlockTime()) > nMDTimeLimit)  ){
    bnBitBreak = bnPowLimit;
    return bnBitBreak.GetCompact();
}

if ( ( (pindexLast->nHeight+1) > 11324612 ) && ( (pblock->nTime - pindexLast->GetBlockTime()) > nLongTimeLimit)  ){
    bnBitBreak = bnBitBreak*15;

     if (bnBitBreak > bnPowLimit)
         bnBitBreak = bnPowLimit;

    return bnBitBreak.GetCompact();
}


	
    // Only change once per interval
    if ((pindexLast->nHeight+1) % nInterval != 0)
    {
        // Special difficulty rule for testnet:
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
		
        return pindexLast->nBits;
    }
	
    // Fastcoin: This fixes an issue where a 51% attack can change difficulty at will.
    // Go back the full period unless it's the first retarget after genesis. Code courtesy of Art Forz
    int blockstogoback = nInterval-1;
    if ((pindexLast->nHeight+1) != nInterval)
        blockstogoback = nInterval;
	
    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < blockstogoback; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);
	
    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    //printf("  nActualTimespan = %"PRI64d"  before bounds\n", nActualTimespan);
	
	if ((pindexLast->nHeight+1) < 1250)
	{
		if (nActualTimespan < nTargetTimespan/32)
			nActualTimespan = nTargetTimespan/32;
	}
	else if ((pindexLast->nHeight+1) < 4000)
	{
		if (nActualTimespan < nTargetTimespan/8)
			nActualTimespan = nTargetTimespan/8;
	}
	else
	{
		if (nActualTimespan < nTargetTimespan/4)
			nActualTimespan = nTargetTimespan/4;
	}
	
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;
	
    // Retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;
	
if (bnNew > bnPowLimit)
    bnNew = bnPowLimit;


return bnNew.GetCompact();
}



unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
   
   int DiffMode = params.DiffMode;

   if (pindexLast->nHeight+1 >= params.DiffModeV2) { DiffMode = 2; }

   if (DiffMode == 2) { return GetNextWorkRequired_V2(pindexLast, pblock, params); }
   if (DiffMode == 1) { return GetNextWorkRequired_V1(pindexLast, pblock, params); }
   
   return GetNextWorkRequired_V2(pindexLast, pblock, params); 
}