# Liar's Deck Solver using External Monte Carlo CFR

This project implements a solver for the 1v1 Liar's Deck game from [Liar's Bar on Steam](https://store.steampowered.com/app/3097560/Liars_Bar/), using External Sampling Monte Carlo Counterfactual Regret Minimization (External MCCFR) to compute approximate Nash equilibrium strategies.

It also includes a Python script to view the generated strategy.

# What It Does

Models the full 1v1 game tree as an imperfect information extensive-form game.

Uses External MCCFR to compute a strategy close to the Nash Equilibrium. 

Computes the exploitability of the strategy

# Results

The game consists of 421 nodes.

The first player to act has a 47.35% chance to win the game. One possible reason for this is because when both players are dealt poor hands (0 or 1 cards of the table) both of their optimal strategies involve frequent bluffing as well as frequent calling. Since Player 1 is forced to play a card first, Player 2 will have the chance to call hence frequently winning the game in these weak-handed nodes.

A computed strategy is provided after 76,000,000 iterations with an exploitability of .05% 

![Output of strategy_viewer.py](https://raw.githubusercontent.com/w451/LiarsDeckSolver/refs/heads/master/BasicCFR/initial_strategy.PNG "Optimal strategy for Player 1's first turn")

# Analysis of Optimal Strategy

The equilibrium strategy involves mostly (>99%) only calling and playing 1 card, except in the case of holding 5 cards where the player must play more than 1 card to ensure that they will win on their second turn.
