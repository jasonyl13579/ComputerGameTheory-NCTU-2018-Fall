#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <math.h>
#include "board.h"
#include "action.h"
#include "pattern.h"
#include "weight.h"
#include <fstream>
int hint = 0;
class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * base agent for agents with weight tables
 */
class weight_agent : public agent {
public:
	weight_agent(const std::string& args = "") : agent(args), alpha(0.003125f), patterns(initial_state()) {
		if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		else{
			init_weights("simple");
			alpha = 0.003125f;
		}
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

protected:
	virtual void init_weights(const std::string& info) {
		//net.emplace_back(65536); // create an empty weight table with size 65536
		//net.emplace_back(65536); // create an empty weight table with size 65536
		// now net.size() == 2; net[0].size() == 65536; net[1].size() == 65536
		pattern p(info);
		std::cout << "player:" + info << std::endl;
		patterns = p;
		if (info == "enhance") alpha = 0.003125f;//alpha = 0.0005208;
		for (size_t i=0; i<patterns.size(); i++){
			net.emplace_back(pow(16, patterns[i].size()));
		}
		//std::cout << patterns[1][0] << std::endl;
		//std::cout << patterns[2][0] << std::endl;
		//std::cout << net[3].size() << std::endl;
	}
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}
	static pattern initial_state() {
		return {};
	}
protected:
	std::vector<weight> net;
	float alpha;
	pattern patterns;
};

/**
 * base agent for agents with a learning rate
 */
class learning_agent : public agent {
public:
	learning_agent(const std::string& args = "") : agent(args), alpha(0.05f) {
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~learning_agent() {}

protected:
	float alpha;
};

/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}),line({ 0, 1, 2, 3}),count(0){
			for (int i=0 ; i<4 ; i++) {popup.push_back(1);popup.push_back(2);popup.push_back(3);}
			std::random_shuffle ( popup.begin(), popup.end() );
		}
	virtual void open_episode(const std::string& flag = "") {
		count = 0;
		bonus_tile_count = 0;
		bonus_tile_valid = false;
		popup.clear();
		for (int i=0 ; i<4 ; i++) {popup.push_back(1);popup.push_back(2);popup.push_back(3);}
		std::random_shuffle ( popup.begin(), popup.end() );
		hint = popup.back();
		popup.pop_back();
	}	
	virtual action take_action(const board& after) {
		//std::cout << after.info();
		//std::cout << popup.size() << std::endl;
		std::cout << after;
		if (popup.size() == 0) {
			for (int i=0 ; i<4 ; i++) {popup.push_back(1);popup.push_back(2);popup.push_back(3);}
			std::random_shuffle ( popup.begin(), popup.end() );
		}
		board::data d = after.info();
		switch (d.previous_dir){
				case 1: // left
					line = {3, 7, 11, 15};
					break;
				case 2: // right
					line = {0, 4, 8, 12};
					break;
				case 3: // up
					line = {12, 13, 14, 15};
					break;
				case 4: // down
					line = {0, 1, 2, 3};
					break;
				case 0: // place
				default:
					std::shuffle(space.begin(), space.end(), engine);
					for (int pos : space) {	
						if (after(pos) != 0) continue;					
						board::cell tile = popup.back();
						popup.pop_back();
						count ++;
						action a  = action::place(pos, hint);
						hint = tile;
						return a;
					}
					return action();
		}	
		std::shuffle(line.begin(), line.end(), engine);
		if  (!bonus_tile_valid){
			if (after.get_max_tile() >= 7) bonus_tile_valid = true;
		}
		
		for (int pos : line) {	
			if (after(pos) != 0) continue;
			if (bonus_tile_valid && (count/21) > bonus_tile_count){
				int x = rand() % 21;
				if (x == 0){
					const int bonus_num = after.get_max_tile() - 6; // first valid tile-48 (index 7)
					int r = rand() % bonus_num;
					//std::array<int, bonus_num> bonus_choose;
					//for (int i=0 ; i<bonus_num ; i++) bonus_choose.at(i) = i;
					//int r = rand() % bonus_choose;
					board::cell tile = r + 4; // minimum tile-6 (index 4)
					count ++;
					bonus_tile_count++;
					action a  = action::place(pos, hint);
					hint = tile;
					return a;
				}
			}
			board::cell tile = popup.back();
			popup.pop_back();
			count ++;
			//if (count == 3) count = 0;
			action a  = action::place(pos, hint);
			hint = tile;
			return a;
		}
		return action();
	}

private:
	std::array<int, 16> space;
	std::array<int, 4> line;
	//std::array<int, 12> popup;
	std::vector<int> popup;
	int count;
	bool bonus_tile_valid = false;
	int bonus_tile_count = 0;
};

/**
 * weight player
 * select a legal action by TDlearning
 */
class player : public weight_agent {
public:
	player(const std::string& args = "") : weight_agent("name=TD role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		
		int max_idx = 0;
		float max_reward = -1000000;
		bool vaild = false;
		board hold;
		for (int op : opcode) {
			board after = board(before).slide_with_board(op);
			if (after == board()) continue;
			vaild = true;
			float value = after.evaluation(patterns, net);
			//if (after.info().rewards + value < 0) std::cout << after.info().rewards + value << std::endl;
			if (after.info().rewards + value > max_reward){
				hold = after;
				max_reward = after.info().rewards + value;
				max_idx = op;
			}
		}
		if (vaild) {
			states.push_back(hold);
			return action::slide(max_idx);
		}
		else {
			//std::cout << before << std::endl;
			return action();
		}
		/*if(max_reward != -1) {
			if (previous.info().modify != -1) previous.upgrade_weight( previous_value, max_reward, net, patterns, alpha);
			previous = hold;
			previous_value = hold_value;
			return action::slide(max_idx);
		}else{
			previous.upgrade_weight( previous_value, -1, net, patterns, alpha);
			return action();
		}*/
	}
	
	void close_episode(const std::string& flag = "") {
		
		backward_training();
		//std::cout << states.size();
		states.clear();
		//forward_training();
	}
	void backward_training(){
		board before_state = states.back();
		board after_state = {};
		before_state.upgrade_weight(-1, net, patterns, alpha);
		states.pop_back();
		//if (after_state == before_state) std::cout << "fuck\n";
		int count = 0;
		while (!states.empty()){
			//cout << before_state;
			count++;
			after_state = before_state;
			before_state = states.back();
			float current_value = after_state.evaluation(patterns, net) + after_state.info().rewards;
			before_state.upgrade_weight( current_value, net, patterns, alpha);
			states.pop_back();
		}	
		//std::cout << count << "\n";
	}
	void forward_training(){
		return;
	}
private:
	std::array<int, 4> opcode;
	std::vector<board> states;
};
