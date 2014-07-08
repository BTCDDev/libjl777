// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define CURRENCY_NAME_BASE0                              "privateNXT"
#define CURRENCY_NAME_SHORT_BASE0                        "pNXT"
#define TOTAL_MONEY_SUPPLY0                              ((uint64_t)100000000L*100000000L)
#define DONATIONS_SUPPLY0                                (0*TOTAL_MONEY_SUPPLY/100)
#define EMISSION_CURVE_CHARACTER0                        8  //23
#define DEFAULT_FEE0                                     ((uint64_t)0) // pow(10, 8)
#ifdef TESTNET
#define P2P_DEFAULT_PORT0                                17770
#define RPC_DEFAULT_PORT0                                17771
#else
#define P2P_DEFAULT_PORT0                                7770
#define RPC_DEFAULT_PORT0                                7771
#endif

#define CURRENCY_NAME_BASE0                              "privateNXT"
#define CURRENCY_NAME_SHORT_BASE0                        "pNXT"
#define CONF_FILENAME0                                   "pNXT.conf"
#define TOTAL_MONEY_SUPPLY0                              ((uint64_t)100000000L*100000000L)
#define DONATIONS_SUPPLY0                                (0*TOTAL_MONEY_SUPPLY/100)
#define EMISSION_CURVE_CHARACTER0                        8  //23
#define DEFAULT_FEE0                                     ((uint64_t)0) // pow(10, 8)
#ifdef TESTNET
#define P2P_DEFAULT_PORT0                                17770
#define RPC_DEFAULT_PORT0                                17771
#else
#define P2P_DEFAULT_PORT0                                7770
#define RPC_DEFAULT_PORT0                                7771
#endif

#define CURRENCY_NAME_BASE1                              "test1"
#define CURRENCY_NAME_SHORT_BASE1                        "t1"
#define CONF_FILENAME1                                   "t1.conf"
#define TOTAL_MONEY_SUPPLY1                              ((uint64_t)100000000L*100000000L)
#define DONATIONS_SUPPLY1                                (0*TOTAL_MONEY_SUPPLY/100)
#define EMISSION_CURVE_CHARACTER1                        8  //23
#define DEFAULT_FEE1                                     ((uint64_t)0) // pow(10, 8)
#ifdef TESTNET
#define P2P_DEFAULT_PORT1                                17772
#define RPC_DEFAULT_PORT1                                17773
#else
#define P2P_DEFAULT_PORT1                                7772
#define RPC_DEFAULT_PORT1                                7773
#endif

#define CURRENCY_NAME_BASE2                              "test2"
#define CURRENCY_NAME_SHORT_BASE2                        "t2"
#define CONF_FILENAME2                                   "t2.conf"
#define TOTAL_MONEY_SUPPLY2                              ((uint64_t)100000000L*100000000L)
#define DONATIONS_SUPPLY2                                (0*TOTAL_MONEY_SUPPLY/100)
#define EMISSION_CURVE_CHARACTER2                        8  //23
#define DEFAULT_FEE2                                     ((uint64_t)0) // pow(10, 8)
#ifdef TESTNET
#define P2P_DEFAULT_PORT2                                17774
#define RPC_DEFAULT_PORT2                                17775
#else
#define P2P_DEFAULT_PORT2                                7774
#define RPC_DEFAULT_PORT2                                7775
#endif

#define NUM_COINTYPES 3
extern char *CURRENCY_NAME_BASE,*CURRENCY_NAME_SHORT_BASE;
extern int COINTYPE,P2P_DEFAULT_PORT,RPC_DEFAULT_PORT;
extern uint64_t TOTAL_MONEY_SUPPLY;
extern uint64_t DONATIONS_SUPPLY;
extern uint64_t EMISSION_CURVE_CHARACTER;
extern uint64_t DEFAULT_FEE;
extern char *CURRENCY_POOLDATA_FILENAME,*CURRENCY_BLOCKCHAINDATA_FILENAME,*CURRENCY_BLOCKCHAINDATA_TEMP_FILENAME,*P2P_NET_DATA_FILENAME,*MINER_CONFIG_FILE_NAME,*CONF_FILENAME2;


#define CURRENCY_MAX_BLOCK_NUMBER                     500000000
#define CURRENCY_MAX_BLOCK_SIZE                       500000000  // block header blob limit, never used!
#define CURRENCY_MAX_TX_SIZE                          1000000000
#define CURRENCY_PUBLIC_ADDRESS_TEXTBLOB_VER          0
#define CURRENCY_PUBLIC_ADDRESS_BASE58_PREFIX         1 // addresses start with "1"
#define CURRENCY_MINED_MONEY_UNLOCK_WINDOW            10
#define CURRENT_TRANSACTION_VERSION                   1
#define CURRENT_BLOCK_MAJOR_VERSION                   1
#define CURRENT_BLOCK_MINOR_VERSION                   0
#define CURRENCY_BLOCK_FUTURE_TIME_LIMIT              60*60*2

#define BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW             60

// TOTAL_MONEY_SUPPLY - total number coins to be generated
#define EMISSION_SUPPLY                               (TOTAL_MONEY_SUPPLY - DONATIONS_SUPPLY)



#define CURRENCY_TO_KEY_OUT_RELAXED                   0
#define CURRENCY_TO_KEY_OUT_FORCED_NO_MIX             1

#define CURRENCY_REWARD_BLOCKS_WINDOW                 400
#define CURRENCY_BLOCK_GRANTED_FULL_REWARD_ZONE       30000 //size of block (bytes) after which reward for block calculated using block size
#define CURRENCY_COINBASE_BLOB_RESERVED_SIZE          600
#define CURRENCY_DISPLAY_DECIMAL_POINT                8

// COIN - number of smallest units in one coin
#define COIN                                            ((uint64_t)100000000) // pow(10, 8)
#define DEFAULT_DUST_THRESHOLD                          ((uint64_t)1) // pow(10, 6)

#define ORPHANED_BLOCKS_MAX_COUNT                       100
#define DIFFICULTY_TARGET                               60 // seconds
#define DIFFICULTY_WINDOW                               720 // blocks
#define DIFFICULTY_LAG                                  15  // !!!
#define DIFFICULTY_CUT                                  60  // timestamps to cut after sorting
#define DIFFICULTY_BLOCKS_COUNT                         (DIFFICULTY_WINDOW + DIFFICULTY_LAG)


#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_SECONDS        (DIFFICULTY_TARGET * CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS)
#define CURRENCY_LOCKED_TX_ALLOWED_DELTA_BLOCKS         1

#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN             DIFFICULTY_TARGET //just alias


#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT          10000  //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT              200    //by default, blocks count in blocks downloading
#define CURRENCY_PROTOCOL_HOP_RELAX_COUNT               3      //value of hop, after which we use only announce of new block


#define CURRENCY_MEMPOOL_TX_LIVETIME                    86400 //seconds, one day
#define CURRENCY_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME     604800 //seconds, one week


#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT           1000

#define P2P_LOCAL_WHITE_PEERLIST_LIMIT                  1000
#define P2P_LOCAL_GRAY_PEERLIST_LIMIT                   5000

#define P2P_DEFAULT_CONNECTIONS_COUNT                   8
#define P2P_DEFAULT_HANDSHAKE_INTERVAL                  60           //secondes
#define P2P_DEFAULT_PACKET_MAX_SIZE                     50000000     //50000000 bytes maximum packet size
#define P2P_DEFAULT_PEERS_IN_HANDSHAKE                  250
#define P2P_DEFAULT_CONNECTION_TIMEOUT                  5000       //5 seconds
#define P2P_DEFAULT_PING_CONNECTION_TIMEOUT             2000       //2 seconds
#define P2P_DEFAULT_INVOKE_TIMEOUT                      60*2*1000  //2 minutes
#define P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT            5000       //5 seconds
#define P2P_MAINTAINERS_PUB_KEY                         "d2f6bc35dc4e4a43235ae12620df4612df590c6e1df0a18a55c5e12d81502aa7"
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT       70
#define P2P_FAILED_ADDR_FORGET_SECONDS                  (60*60)     //1 hour

#define P2P_IP_BLOCKTIME                                (60*60*24)  //24 hour
#define P2P_IP_FAILS_BEFOR_BLOCK                        10
#define P2P_IDLE_CONNECTION_KILL_INTERVAL               (5*60) //5 minutes


/* This money will go to growth of the project */
  #define CURRENCY_DONATIONS_ADDRESS                     "1Gx7pfdh8aZRUU9paTc37gcXUWbYxcqbu814DgAdHxdKGAeHLdYHKS13B5SoC9j2Zv9BvkzPik53nS5nyPiiaoDqQpSs6Z1"
  #define CURRENCY_DONATIONS_ADDRESS_TRACKING_KEY        "03960414e9441d40ada9af54ef6c9865394ed796c8eec81ee94eb87d53ec7c0e"
/* This money will go to the founder of CryptoNote technology, 10% of donations */
  #define CURRENCY_ROYALTY_ADDRESS                       "1JnCpSjCFwTDcDwoU3BJsqUC1kn5EChEpA6Bi5kYfd1qMPCbHddDs8FD2bd2d5BvrG6MKzXLcTQ8JdmnmZ4DaLDYL6FEHv6"
  #define CURRENCY_ROYALTY_ADDRESS_TRACKING_KEY          "8b034b7cad7a1d2097838370dcdd601864542c5f75f155b834c397a96270ca00"





#ifdef TESTNET
#define CURRENCY_DONATIONS_INTERVAL                     10
#else
#define CURRENCY_DONATIONS_INTERVAL                     720
#endif


#define ALLOW_DEBUG_COMMANDS

//#ifndef TESTNET
#define CURRENCY_NAME                                   CURRENCY_NAME_BASE
#define CURRENCY_NAME_SHORT                             CURRENCY_NAME_SHORT_BASE
//#else
//#define CURRENCY_NAME                                   CURRENCY_NAME_BASE"_testnet"
//#define CURRENCY_NAME_SHORT                             CURRENCY_NAME_SHORT_BASE"_testnet"
//#endif

#define _CURRENCY_POOLDATA_FILENAME                      "poolstate.bin"
#define _CURRENCY_BLOCKCHAINDATA_FILENAME                "blockchain.bin"
#define _CURRENCY_BLOCKCHAINDATA_TEMP_FILENAME           "blockchain.bin.tmp"
#define _P2P_NET_DATA_FILENAME                           "p2pstate.bin"
#define _MINER_CONFIG_FILE_NAME                          "miner_conf.json"

