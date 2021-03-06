//
//  main.cpp
//  BlockSim
//
//  Created by Harry Kalodner on 6/6/16.
//  Copyright © 2016 Harry Kalodner. All rights reserved.
//

#include "strategy.hpp"
#include "utils.hpp"
#include "miner.hpp"
#include "block.hpp"
#include "blockchain.hpp"
#include "minerStrategies.h"
#include "logging.h"
#include "game.hpp"
#include "minerGroup.hpp"
#include "minerStrategies.h"
#include "game_result.hpp"
#include "miner_result.hpp"
#include "mining_style.hpp"
#include "minerImp.hpp"

#include <cassert>
#include <iostream>
#include <fstream>
#include <cmath>
#include <queue>
#include <random>
#include <algorithm>
#include <vector>
#include <map>


#define NOISE_IN_TRANSACTIONS false //miners don't put the max value they can into a block (simulate tx latency)

#define NETWORK_DELAY BlockTime(0)         //network delay in seconds for network to hear about your block
#define EXPECTED_NUMBER_OF_BLOCKS BlockCount(10000)

#define LAMBERT_COEFF 0.13533528323661//coeff for lambert func equil  must be in [0,.2]
//0.13533528323661 = 1/(e^2)

#define B BlockValue(3.125) // Block reward
//#define TOTAL_BLOCK_VALUE BlockValue(15.625)
#define TOTAL_BLOCK_VALUE BlockValue(13)

#define SEC_PER_BLOCK BlockRate(600)     //mean time in seconds to find a block

//#define B BlockValue(3.125) // Block reward
#define A (TOTAL_BLOCK_VALUE - B)/SEC_PER_BLOCK  //rate transactions come in

#define SELFISH_GAMMA 0.0 //fraction of network favoring your side in a dead tie
//half way and miners have equal hash power

int main(int, const char * []) {
    
    int numberOfGames = 2000;
    
    //#########################################################################################
    //idea of simulation: 2 miners, only an honest, and a selfish miner. Run many games, with the
    //size of the two changing. Plot the expected profit vs. actual profit. (reproduce fig 2 in selfish paper)
    GAMEINFO("#####\nRunning Selfish Mining Simulation\n#####" << std::endl);
    std::ofstream plot;
    plot.open("selfishMiningPlot.txt");
    
    //start running games
    for (int gameNum = 1; gameNum < numberOfGames; gameNum++) {
        
        std::vector<std::unique_ptr<Miner>> miners;
        
        // Scale power to reach %50 on the last game
        HashRate selfishPower = HashRate(.5 * (1.0 / numberOfGames) * gameNum);
        auto defaultStrat = createDefaultSelfishStrategy(NOISE_IN_TRANSACTIONS, SELFISH_GAMMA);
        auto selfishStrat = createCleverSelfishStrategy(NOISE_IN_TRANSACTIONS, Value(100));
        
        MinerParameters selfishMinerParams = {0, std::to_string(0), selfishPower, NETWORK_DELAY, COST_PER_SEC_TO_MINE};
        MinerParameters defaultinerParams = {1, std::to_string(1), HashRate(1.0) - selfishPower, NETWORK_DELAY, COST_PER_SEC_TO_MINE};
        
        miners.push_back(std::make_unique<Miner>(selfishMinerParams, selfishStrat.createImp()));
        miners.push_back(std::make_unique<Miner>(defaultinerParams, defaultStrat.createImp()));
        
        MinerGroup minerGroup(std::move(miners));
        
        GAMEINFO("\n\nGame#: " << gameNum << " The board is set, the pieces are in motion..." << std::endl);
        GAMEINFO("miner ratio:" << selfishPower << " selfish." << std::endl);
        
        BlockchainSettings blockchainSettings = {SEC_PER_BLOCK, A, B};
        GameSettings gameSettings = {EXPECTED_NUMBER_OF_BLOCKS, blockchainSettings};
        
        auto result = runGame(minerGroup, gameSettings);
        auto minerResults = result.second.minerResults;
        
        GAMEINFO("The game is complete. Calculate the scores:" << std::endl);
        
        //calculate the score at the end
        BlockCount totalBlocks = BlockCount(0);
        BlockCount finalBlocks = BlockCount(0);
        
        for (const auto &miner : minerGroup.miners) {
            totalBlocks += miner->getBlocksMinedTotal();
            finalBlocks += minerResults[miner.get()].blocksInWinningChain;
        }
        
        const Block &winningBlock = result.first->winningHead();
        
        Value totalProfit = winningBlock.valueInChain;
        GAMEINFO("Total profit:" << totalProfit << std::endl);
        
        auto fractionOfProfits = minerResults[minerGroup.miners[0].get()].totalProfit/totalProfit;
        GAMEINFO("Fraction earned by selfish:" << fractionOfProfits << " with " << selfishPower << " fraction of hash power" << std::endl);
        plot << selfishPower << " " << fractionOfProfits << std::endl;
        
    }
    
    GAMEINFO("Games over." << std::endl);
}
