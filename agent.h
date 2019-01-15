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
extern int HINT_YM = 0;
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

class state_type {
public:
	enum type : char {
		before  = 'b',
		after   = 'a',
		illegal = 'i'
	};

public:
	state_type() : t(illegal) {}
	state_type(const state_type& st) = default;
	state_type(state_type::type code) : t(code) {}

	/*friend std::istream& operator >>(std::istream& in, state_type& type) {
		std::string s;
		if (in >> s) type.t = static_cast<state_type::type>((s + " ").front());
		return in;
	}

	friend std::ostream& operator <<(std::ostream& out, const state_type& type) {
		return out << char(type.t);
	}*/

	bool is_before()  const { return t == before; }
	bool is_after()   const { return t == after; }
	bool is_illegal() const { return t == illegal; }

private:
	type t;
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
		int hint_num = 1; 
		if (info == "enhance_hint") {
			alpha = 0.003125f;//alpha = 0.0005208;
			hint_num = 5;
		}
		for (size_t i=0; i<patterns.size(); i++){
			net.emplace_back(pow(16, patterns[i].size())* hint_num); //*5 = HINT_YM
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
		std::cout << "save_weights" << std::endl;
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) {
			std::cout << "error" << std::endl;
			std::exit(-1);
		}
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

class rndenv : public weight_agent {
public:
	rndenv(const std::string& args = "") : weight_agent("name=TD_ENV role=environment " + args),
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
		HINT_YM = popup.back();
		popup.pop_back();
	}	
	virtual action take_action(const board& after) {
		//std::cout << after.info();
		//std::cout << popup.size() << std::endl;
		//std::cout << after;
		
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
					std::random_shuffle ( space.begin(), space.end() );
					for (int pos : space) {	
						if (after(pos) != 0) continue;					
						board::cell tile = popup.back();
						popup.pop_back();
						count ++;
						action a  = action::place(pos, HINT_YM);
						HINT_YM = tile >= 4 ? 4 : tile;
						return a;
					}
					return action();
		}	
		//std::random_shuffle ( line.begin(), line.end() );
		if  (!bonus_tile_valid){
			if (after.get_max_tile() >= 7) bonus_tile_valid = true;
		}
		int min_idx = -1;
		int idx = -1;
		float min_reward = 1000000;
		float min_hint = -1;
		board::cell tile_hint = HINT_YM;
		if (HINT_YM == 4){
			const int bonus_num = after.get_max_tile() - 6; // first valid tile-48 (index 7)
			int r = rand() % bonus_num;
			tile_hint = r + 4; // minimum tile-6 (index 4)
			//std::cout << count << ":" << tile_hint << std::endl;
		}
		int minimax_count = count+1;
		int minimax_bonus_count = bonus_tile_count;
		for (int pos : line) {
			idx ++;
			if (after(pos) != 0) continue;
			for (int i=1; i<5; i++){
				vector<int>::iterator it = find(popup.begin(), popup.end(), i);
				if (it != popup.end() || (i == 4 && bonus_tile_valid && (count/21) > bonus_tile_count)){
					board before(after);
					before.hint(i);
					action::place(pos, tile_hint).apply(before);
					float value = minimax(before, state_type::before, popup, 4, minimax_count, minimax_bonus_count + (i == 4) ? 1 : 0);
					if (value < min_reward){
						min_reward = value;
						min_idx = idx;
						min_hint = i;
					}
				}
			}
		}
		//std::cout << min_idx << std::endl;
		if (min_idx == -1) return action();
		if (min_hint != 4){
			vector<int>::iterator it = find(popup.begin(), popup.end(), min_hint);
			popup.erase(it);
		}else bonus_tile_count++;
		HINT_YM = min_hint;
		count ++;
		return action::place(line[min_idx], tile_hint);
	}
	float minimax(board current, state_type t, std::vector<int> bag, int layer_iter, int minimax_count, int minimax_bonus_count){
		layer_iter -- ;
		if (t.is_before()){
			//std::cout << "test" << std::endl;
			float max_reward = -1000000;
			std::array<int, 4> opcode = { 0, 1, 2, 3 };
			for (int op : opcode) {
				board after = board(current).slide_with_board(op);
				if (after == board()) continue;
				after.hint(current.hint());
				float value = 0;
				if (layer_iter == 1) value = after.evaluation(patterns, net);
				else value = minimax(after, state_type::after, bag, layer_iter, minimax_count, minimax_bonus_count);
				//std::cout << value << std::endl;
				if (after.info().rewards + value > max_reward){
					max_reward = after.info().rewards + value;
				}
			}
			return max_reward;
		}else if(t.is_after()){
			minimax_count++;
			if (bag.size() == 0) {
				for (int i=0 ; i<4 ; i++) {bag.push_back(1);bag.push_back(2);bag.push_back(3);}
			}
			board::data d = current.info();
			std::array<int, 4> line2;
			switch (d.previous_dir){
					case 1: // left
						line2 = {3, 7, 11, 15};
						break;
					case 2: // right
						line2 = {0, 4, 8, 12};
						break;
					case 3: // up
						line2 = {12, 13, 14, 15};
						break;
					case 4: // down
						line2 = {0, 1, 2, 3};
						break;
					case 0: // place
					default:
						break;
			}	
			
			int idx = -1;
			float min_reward = 1000000;
			//float min_hint = -1;
			
			board::cell tile_hint = current.hint();
			if (HINT_YM == 4){
				const int bonus_num = current.get_max_tile() - 6; // first valid tile-48 (index 7)
				int r = rand() % bonus_num;
				tile_hint = r + 4; // minimum tile-6 (index 4)
				//std::cout << count << ":" << tile_hint << std::endl;
			}
			for (int pos : line2) {
				idx ++;
				if (current(pos) != 0) continue;
				for (int i=1; i<5; i++){
					vector<int>::iterator it = find(bag.begin(), bag.end(), i);
					if (it != bag.end() || (i == 4 && bonus_tile_valid && (minimax_count/21) > minimax_bonus_count)){
						board before(current);
						before.hint(i);
						action::place(pos, tile_hint).apply(before);
						std::vector<int> after_bag = bag;
						if (i != 4){
							vector<int>::iterator it = find(after_bag.begin(), after_bag.end(), i);
							after_bag.erase(it);
						}
						float value = minimax(before, state_type::before, after_bag, layer_iter, minimax_count, minimax_bonus_count + (i == 4) ? 1 : 0);
						/*float value = 0;
						if (tile_hint == 4){
							const int bonus_num = current.get_max_tile() - 6; // first valid tile-48 (index 7)
							//int r = rand() % bonus_num;
							for (int j= 0; j< bonus_num; j++){
								action::place(pos, j + 4).apply(before);
								value += minimax(before, state_type::before, bag, layer_iter, minimax_count, minimax_bonus_count);
							}
							value/= bonus_num;
						}else{
							action::place(pos, tile_hint).apply(before);
							value = minimax(before, state_type::before, bag, layer_iter, minimax_count, minimax_bonus_count);
						}*/
						if (value < min_reward){
							min_reward = value;
						}
					}
				}
			}
			
			return min_reward;
		}
		return 0;
	}
private:
	std::array<int, 16> space;
	std::array<int, 4> line;
	std::vector<int> popup;
	int count;
	bool bonus_tile_valid = false;
	int bonus_tile_count = 0;
};
/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
/*class rndenv : public random_agent {
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
		HINT_YM = popup.back();
		popup.pop_back();
	}	
	virtual action take_action(const board& after) {
		//std::cout << after.info();
		//std::cout << popup.size() << std::endl;
		//std::cout << after;
		
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
						action a  = action::place(pos, HINT_YM);
						HINT_YM = tile >= 4 ? 4 : tile;
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
					//std::cout << "Fuck:" << tile << std::endl;
					action a  = action::place(pos, HINT_YM);
					HINT_YM = tile >= 4 ? 4 : tile;
					//HINT_YM = tile;
					return a;
				}
			}
			board::cell tile = popup.back();
			popup.pop_back();
			count ++;
			//if (count == 3) count = 0;
			action a  = action::place(pos, HINT_YM);
			HINT_YM = tile >= 4 ? 4 : tile;
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
*/
/**
 * weight player
 * select a legal action by TDlearning
 */
class player : public weight_agent {
public:
	player(const std::string& args = "") : weight_agent("name=TD role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before) {
		//before.HINT_YM(HINT_YM);
		int max_idx = 0;
		float max_reward = -1000000;
		bool vaild = false;
		board hold;
		for (int op : opcode) {
			board after = board(before).slide_with_board(op);
			if (after == board()) continue;
			vaild = true;
			after.hint(HINT_YM);
			float value = after.evaluation(patterns, net);
			//if (after.info().rewards + value < 0) std::cout << after.info().rewards + value << std::endl;
			if (after.info().rewards + value > max_reward){
				hold = after;
				max_reward = after.info().rewards + value;
				max_idx = op;
			}
		}
		if (vaild) {
			//std::cout << HINT_YM << std::endl;
			hold.hint(HINT_YM);
			states.push_back(hold);
			return action::slide(max_idx);
		}
		else {
			//std::cout << before << std::endl;
			return action();
		}
	}
	
	void close_episode(const std::string& flag = "") {
		
		backward_training();
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
			//std::cout << after_state.HINT_YM() << std::endl;
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
