#include <map>
#include <algorithm>
#include <random>
#include <nlohmann/json.hpp>

using namespace std;
using namespace nlohmann;

//Precomputed distributions of different hands being dealt to Player 1
double p1_distr[6] = { 0.05108359133126935, 0.25541795665634676, 0.3973168214654283, 0.23839009287925697, 0.05417956656346749, 0.00361197110423117 };

//Precomputed distributions of different hands being dealt to Player 2 given what hand was dealt to Player 1
double p2_distr[6][6] = { {0.00699300699300699, 0.09324009324009325, 0.32634032634032634,
  0.39160839160839161, 0.16317016317016317, 0.01864801864801865},
 {0.01864801864801865, 0.16317016317016317, 0.39160839160839161,
  0.32634032634032634, 0.09324009324009325, 0.00699300699300699},
 {0.04195804195804196, 0.25174825174825177, 0.41958041958041958,
  0.23976023976023977, 0.04495504495504495, 0.00199800199800200},
 {0.08391608391608392, 0.34965034965034963, 0.39960039960039961,
  0.14985014985014986, 0.01665001665001665, 0.00033300033300033},
 {0.15384615384615385, 0.43956043956043955, 0.32967032967032966,
  0.07326007326007326, 0.00366300366300366, 0.00000000000000000},
 {0.26373626373626374, 0.49450549450549453, 0.21978021978021978,
  0.02197802197802198, 0.00000000000000000, 0.00000000000000000} };

double getRandom() {
	static random_device rd;
	static mt19937 generator(rd());
	static uniform_real_distribution<double> distribution(0.0, 1.0);

	return distribution(generator);
}

struct LiarsNode {

	const bool use_plus_regret = false;
	const double alpha = 1.5;
	const double beta = 0;
	const double gamma = 2;

	int action_count;
	int observed_action_count;
	int range_size;

	double*** regret_sum;
	double*** strategy;
	double*** strategy_sum;
	LiarsNode** children;

	int player;
	int num_cards;
	bool is_call_node;
	bool is_call_only;
	bool can_call;
	int total_node_count;
	int max_play_count;

	LiarsNode(int p, int mine, int opponents, bool call = false) : player(p), num_cards(mine), is_call_node(call), total_node_count(1){

		can_call = !is_call_node && (p != 0 || num_cards != 5);
		max_play_count = std::min(num_cards, 3);

		//Number of different actions (1 good 1 bad, badgood goodgood badbad)
		int count[5] = { 2,5,9,9,9 };

		is_call_only = opponents == 0;
		
		observed_action_count = (int)can_call + max_play_count; //Observers will see us play 1-n cards or call
		range_size = num_cards + 1;

		if (!is_call_node) {
			if (!is_call_only) {
				action_count =  (int)can_call + count[max_play_count - 1];
			} else {
				observed_action_count = 1;
				action_count = 1;
			}
			regret_sum = new double** [action_count];
			strategy_sum = new double** [action_count];			
			strategy = new double** [action_count];
			children = new LiarsNode * [observed_action_count];
			
		} else {
			action_count = 0;
		}

		if (!is_call_only) {
			for (int i = 0; i < max_play_count; i++) {
				children[i] = new LiarsNode(p ^ 1, opponents, num_cards - i - 1);
				total_node_count += children[i]->total_node_count;
			}
			if (can_call) {
				children[max_play_count] = new LiarsNode(p ^ 1, 0, 0, true);
				total_node_count += children[max_play_count]->total_node_count;
			}
		} else if (can_call) {
			children[0] = new LiarsNode(p ^ 1, 0, 0, true);
			total_node_count += children[0]->total_node_count;
		}

		if (!is_call_node) {
			for (int i = 0; i < action_count; i++) {
				strategy_sum[i] = new double*[6];
				regret_sum[i] = new double*[6];
				strategy[i] = new double*[6];
				for (int j = 0; j < 6; j++) {

					strategy_sum[i][j] = new double[range_size];				
					regret_sum[i][j] = new double[range_size];
					strategy[i][j] = new double[range_size];
					for (int k = 0; k < range_size; k++) {
						regret_sum[i][j][k] = 0;
						strategy_sum[i][j][k] = 0;
						strategy[i][j][k] = 0;
					}
				}
			}
		}
	}

	~LiarsNode(){	
		if (!is_call_node) {
			for (int i = 0; i < observed_action_count; i++) {
				delete children[i];
			}
			delete[] children;

			for (int i = 0; i < action_count; i++) {
				for (int j = 0; j < 6; j++) {
					delete[] strategy_sum[i][j];
					delete[] strategy[i][j];
					delete[] regret_sum[i][j];
				}
				delete[] strategy_sum[i];
				delete[] strategy[i];
				delete[] regret_sum[i];
			}
			delete[] strategy_sum;
			delete[] strategy;
			delete[] regret_sum;
		}
	}

	int MapToActionNumber(int goods) {
		return ((max_play_count - 1) * (max_play_count + 2)) / 2 + goods;
	}

	std::pair<int, int> ActionConvert(int action) {
		if (can_call && action == action_count - 1) {
			return { -1, -1 };
		}

		static const std::pair<int, int> action_map[] = {
			{1,0}, {1,1},
			{2,0}, {2,1}, {2,2},
			{3,0}, {3,1}, {3,2}, {3,3},
			{4,0}, {4,1}, {4,2}, {4,3}, {4,4},
			{5,0}, {5,1}, {5,2}, {5,3}, {5,4}, {5,5}
		};

		constexpr int map_size = sizeof(action_map) / sizeof(action_map[0]);

		if (action >= 0 && action < map_size) {
			return action_map[action];
		}

		return { -1, -1 };
	}


	int ActionToObservedAction(int a) {
		int i = ActionConvert(a).first;
		if (i == -1) {
			return observed_action_count - 1;
		} else {
			return i - 1;
		}
	}

	bool ObservedActionIsCall(int o) {
		return can_call && o == observed_action_count - 1;
	}

	bool CanActionBePlayed(int a, int goods) {
		if(can_call && a == action_count - 1){
			return true;
		}

		pair<int, int> result = ActionConvert(a);

		int my_bads = num_cards - goods;
		int attempted_bads = result.first - result.second;
		return result.first <= max_play_count && result.second <= goods && attempted_bads <= my_bads ;
	}

	vector<vector<int>> action_cache = vector<vector<int>>(6);
	vector<bool> action_cache_mask = vector<bool>(6, false);
	void GetAllLegalActions(int goods, vector<int>& other_actions) {
		if (action_cache_mask[goods]) {
			other_actions = action_cache[goods];
			return;
		}

		for (int a = 0; a < action_count; a++) {
			if (CanActionBePlayed(a, goods)) {
				other_actions.push_back(a);
			}
		}

		action_cache[goods] = other_actions;
		action_cache_mask[goods] = true;
	}

	void UpdateStrategy(int initial_count, int current_count) {
		double total_pos_regret = 0.0;

		int legal_count = 0;

		vector<int> actions;
		GetAllLegalActions(current_count, actions);

		for (int a : actions) {
			total_pos_regret += std::max(regret_sum[a][initial_count][current_count], 0.0);
			legal_count++;
		}

		if (total_pos_regret > 0) {
			for (int a : actions) {
				strategy[a][initial_count][current_count] = std::max(regret_sum[a][initial_count][current_count], 0.0) / total_pos_regret;
			}
		} else {
			for (int a : actions) {
				strategy[a][initial_count][current_count] = 1.0 / legal_count;
			}
		}
	}

	void UpdateCumulativeStrategy(int initial_count, int current_count, int iteration) {
		vector<int> actions;
		GetAllLegalActions(current_count, actions);

		for (int i : actions) {
			strategy_sum[i][initial_count][current_count] +=
				strategy[i][initial_count][current_count] *
				pow((double)iteration, gamma);			
		}
	}

	int SampleLegalAction(int initial_count, int current_count) {
		double random = getRandom();
		double cumul = 0;
		int found_action = 0;

		vector<int> actions;
		GetAllLegalActions(current_count, actions);

		for (int a : actions) {
			if (cumul <= random) {
				found_action = a;
			}
			cumul += strategy[a][initial_count][current_count];
			
		}
		return found_action;
	}

	double child_regrets[21] = { 0 };
	double cfr(int my_initial_count, int opp_initial_count, int update_turn, int goods, int opponent_goods, bool was_last_bluff, int iteration) {
		if (is_call_node) {
			if (update_turn) {
				return was_last_bluff ? -1 : 1;
			} else {
				return was_last_bluff ? 1 : -1;
			}
		} else {
			//We are at a node where we have n cards and our opponent has m cards: look up our range strategy for how many good cards we have and sample it.
			
			double value = 0;
			UpdateStrategy(my_initial_count, goods);

			if (update_turn) {
				UpdateStrategy(my_initial_count, goods);
				vector<int> legal_actions;
				GetAllLegalActions(goods, legal_actions);				

				for (int other : legal_actions) {
					pair<int, int> res2 = ActionConvert(other);
					int observed2 = ActionToObservedAction(other);

					bool calls = res2.first == -1;
					bool is_bluffing = calls ? was_last_bluff : res2.first != res2.second;

					int new_goods = calls ? goods : goods - res2.second;
					int new_cards = calls ? num_cards : num_cards - res2.first;

					child_regrets[other] = children[observed2]->cfr(opp_initial_count, my_initial_count, update_turn ^ 1, opponent_goods, new_goods, is_bluffing, iteration);
					double s = strategy[other][my_initial_count][goods];
					value += child_regrets[other] * s;
				}

				for (int other : legal_actions) {
					int observed2 = ActionToObservedAction(other);
					double& d = regret_sum[other][my_initial_count][goods];
					d += child_regrets[other] - value;

					if (regret_sum[other][my_initial_count][goods] > 0) {
						regret_sum[other][my_initial_count][goods] *= (pow((double)iteration, alpha)) / (pow((double)iteration, alpha) + 1);
					} else {
						regret_sum[other][my_initial_count][goods] *= (pow((double)iteration, beta)) / (pow((double)iteration, beta) + 1);
					}
				}

				UpdateStrategy(my_initial_count, goods);
				UpdateCumulativeStrategy(my_initial_count, goods, iteration);
			} else {				
				vector<int> other_actions;
				int action = SampleLegalAction(my_initial_count, goods);
				pair<int, int> res = ActionConvert(action);
				int observed = ActionToObservedAction(action);

				bool calls = res.first == -1;
				bool is_bluffing = calls ? was_last_bluff : res.first != res.second;

				int new_goods = calls ? goods : goods - res.second;
				int new_cards = calls ? num_cards : num_cards - res.first;

				value = children[observed]->cfr(opp_initial_count, my_initial_count, update_turn ^ 1, opponent_goods, new_goods, is_bluffing, iteration);
			}				
			return value;
		}
	}

	double eval(int initial_count, int opp_initial_count, int goods, int opponent_goods, bool was_last_bluff) {
		if (is_call_node) {
			if (player == 0) {
				return was_last_bluff ? 0 : 1;
			} else {
				return was_last_bluff ? 1 : 0;
			}
		} else {
			vector<int> legal_actions;
			GetAllLegalActions(goods, legal_actions);

			double total = 0.0;
			for (int a : legal_actions) {
				total += strategy_sum[a][initial_count][goods];
			}

			if (total == 0) {
				for (int a : legal_actions) {
					strategy_sum[a][initial_count][goods] = 1;
				}
				total = legal_actions.size();
			}

			double value = 0;
			for (int other : legal_actions) {
				pair<int, int> res2 = ActionConvert(other);
				int observed2 = ActionToObservedAction(other);

				bool calls = res2.first == -1;
				bool is_bluffing = calls ? was_last_bluff : res2.first != res2.second;

				int new_goods = calls ? goods : goods - res2.second;
				int new_cards = calls ? num_cards : num_cards - res2.first;


				double evalval = children[observed2]->eval(opp_initial_count, initial_count, opponent_goods, new_goods, is_bluffing);

				value += evalval
					* (strategy_sum[other][initial_count][goods] / total);
			}

			return value;
		}
	}

	struct RangeItem {
		double p;
		int initial;
		int current;
		bool is_bluff;
	};

	//TODO: We need to make a greedy response against our opponents hand distribution, not their actual hand
	double greedy_eval(int locked_player, vector<RangeItem> locked_range, int o_initial, int o_goods, bool was_o_bluff = false) {
		if (is_call_node) {
			if (locked_player != player) {
				//If we are player 0 and bluffed and got called then 0
				if (player == 0) {
					return was_o_bluff ? 0 : 1;
				} else {
					return was_o_bluff ? 1 : 0;
				}
			} else {
				double value = 0;
				for (const RangeItem& ri : locked_range) {
					double v = 0;
					if (player == 0) {
						v = ri.is_bluff ? 0 : 1;
					} else {
						v = ri.is_bluff ? 1 : 0;
					}

					value += v * ri.p;
				}

				return value;
			}
		} else {
			if (locked_player != player) {
				//Given our current hand compute the EV against our opponents entire range for each action and pick the best action

				int my_initial = o_initial;
				int my_count = o_goods;

				vector<int> legal_actions;
				GetAllLegalActions(my_count, legal_actions);

				double total = 0.0;
				for (int a : legal_actions) {
					total += strategy_sum[a][my_initial][my_count];
				}

				if (total == 0) {
					for (int a : legal_actions) {
						strategy_sum[a][my_initial][my_count] = 1;
					}
					total = legal_actions.size();
				}

				double value = player == 0 ? -INFINITY : INFINITY; //Any other action will always be better than nothing
				double mean = 0;
				for (int other : legal_actions) {

					pair<int, int> res2 = ActionConvert(other);
					int observed2 = ActionToObservedAction(other);

					bool calls = res2.first == -1;
					bool is_bluffing = calls ? was_o_bluff : res2.first != res2.second;

					int new_o_goods = (calls ? my_count : my_count - res2.second);

					double evalval = children[observed2]->greedy_eval(locked_player, locked_range, o_initial, new_o_goods, is_bluffing);
					
					mean += evalval / legal_actions.size();

					if (player == 0) { //Player 1 wants the greatest chance of Player 1 winning
						value = std::max(evalval, value);
					} else { //Player 2 wants the lowest chance of Player 1 winning
						value = std::min(evalval, value);
					}
				}

				return value;
			} else {
				double observed_p[6] = {0};
				vector<RangeItem> observed_to_range[6] = { {} };
				double value = 0.0;
				for (RangeItem ri : locked_range) {
					vector<int> legal_actions;
					GetAllLegalActions(ri.current, legal_actions);

					double total = 0.0;
					for (int a : legal_actions) {
						total += strategy_sum[a][ri.initial][ri.current];
					}

					if (total == 0) {
						for (int a : legal_actions) {
							strategy_sum[a][ri.initial][ri.current] = 1;
						}
						total = legal_actions.size();
					}

					for (int a : legal_actions) {
						pair<int, int> res2 = ActionConvert(a);
						int observed2 = ActionToObservedAction(a);

						observed_p[observed2] += ri.p * (strategy_sum[a][ri.initial][ri.current] / total);

						bool calls = res2.first == -1;

						RangeItem newri = {0};
						newri.initial = ri.initial;
						newri.current = (calls ? ri.current : ri.current - res2.second);
						newri.is_bluff = calls ? was_o_bluff : res2.first != res2.second;
						newri.p = ri.p * (strategy_sum[a][ri.initial][ri.current] / total);

						if (newri.p > 0) {
							//Look for an item like this that already exists and instead add the probabilities
							bool found = false;
							for (RangeItem& cmpri : observed_to_range[observed2]) {
								if (cmpri.initial == newri.initial && cmpri.current == newri.current && cmpri.is_bluff == newri.is_bluff) {
									cmpri.p += newri.p;
									found = true;
									break;
								}
							}

							if (!found) {
								observed_to_range[observed2].push_back(newri);
							}
						}
					}				
				}

				for (int l = 0; l < observed_action_count; l++) {
					vector<RangeItem>& range = observed_to_range[l];
					double total = 0;
					for (const RangeItem& r1 : range) {
						total += r1.p;
					}

					for (RangeItem& r1 : range) {
						if (total > 0) {
							r1.p /= total;
						}
					}
				}
				
				

				for (int l = 0; l < observed_action_count; l++) {
					if (observed_p[l] > 0) {
						bool calls = ObservedActionIsCall(l);
						bool is_bluffing = calls ? was_o_bluff : false;

						vector<RangeItem>& range = observed_to_range[l];
						double evalval = children[l]->greedy_eval(locked_player, range, o_initial, o_goods, is_bluffing);
						value += evalval * observed_p[l];
					}
				}

				
				return value;
			}
		}
	}

	json strategy_to_json() {
		double totals[6][6] = {{0}};
		
		for (int ic = 0; ic <= 5; ic++) {
			for (int current = 0; current < range_size; current++) {
				vector<int> legal_actions;
				GetAllLegalActions(current, legal_actions);

				for (int a : legal_actions) {
					totals[ic][current] += strategy_sum[a][ic][current];
				}
			}
		}
		
		//j[action][initial_count][current_count]
		json j = json::array();
		for (int i = 0; i < action_count; ++i) {
			json row = json::array();
			for (int ic = 0; ic <= 5; ic++) {
				json icrow = json::array();
				for (int j_col = 0; j_col < range_size; ++j_col) {
					icrow.push_back(strategy_sum[i][ic][j_col] / totals[ic][j_col]);
				}
				row.push_back(icrow);
			}
			j.push_back(row);
		}

		return j;
	}

	json get_action_info() {
		json j = json::array();
		for (int i = 0; i < action_count; ++i) {
			pair<int, int> res = ActionConvert(i);
			int observed = ActionToObservedAction(i);

			json row;
			row["play_count"] = res.first;
			row["play_good_count"] = res.second;
			row["is_call"] = res.first == -1;
			row["observed_action"] = observed;
			row["range_mask"] = json::array();

			for (int g = 0; g < range_size; g++) {
				row["range_mask"].push_back(CanActionBePlayed(i, g));
			}
			
			j.push_back(row);
		}

		return j;
	}

	json to_json() {
		json ret;
		ret["player"] = player;
		ret["num_cards"] = num_cards;
		ret["is_call_node"] = is_call_node;
		ret["is_call_only"] = is_call_only;
		ret["can_call"] = can_call;
		ret["action_count"] = action_count;
		ret["observed_action_count"] = observed_action_count;
		ret["range_size"] = range_size;
		ret["node_count"] = total_node_count;

		//Indexed by action then range
		ret["strategy"] = strategy_to_json();
		ret["action_info"] = get_action_info();
		ret["children"] = json::array(); 

		for (int i = 0; i < observed_action_count; i++) {
			ret["children"].push_back(children[i]->to_json());
		}

		return ret;
	}
};