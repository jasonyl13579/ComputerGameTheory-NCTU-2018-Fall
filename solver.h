#pragma once
#include <iostream>
#include <algorithm>
#include <cmath>
#include "board.h"
#include "action.h"
#include <numeric>

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

	friend std::istream& operator >>(std::istream& in, state_type& type) {
		std::string s;
		if (in >> s) type.t = static_cast<state_type::type>((s + " ").front());
		return in;
	}

	friend std::ostream& operator <<(std::ostream& out, const state_type& type) {
		return out << char(type.t);
	}

	bool is_before()  const { return t == before; }
	bool is_after()   const { return t == after; }
	bool is_illegal() const { return t == illegal; }

private:
	type t;
};

class state_hint {
public:
	state_hint(const board& state) : state(const_cast<board&>(state)) {}

	char type() const { return state.hint() ? state.hint() + '0' : 'x'; }
	operator board::cell() const { return state.hint(); }

public:
	friend std::istream& operator >>(std::istream& in, state_hint& hint) {
		while (in.peek() != '+' && in.good()) in.ignore(1);
		char v; in.ignore(1) >> v;
		hint.state.hint(v != 'x' ? v - '0' : 0);
		return in;
	}
	friend std::ostream& operator <<(std::ostream& out, const state_hint& hint) {
		return out << "+" << hint.type();
	}

private:
	board& state;
};


class solver {
public:
	typedef float value_t;

public:
	class answer {
	public:
		answer(value_t min = -1, value_t avg = -1, value_t max = -1) : min(min), avg(avg), max(max) {}
	    friend std::ostream& operator <<(std::ostream& out, const answer& ans) {
	    	return !std::isnan(ans.avg) ? (out << ans.min << " " << ans.avg << " " << ans.max) : (out << "-1") << std::endl;
		}
		answer maximum(answer a, answer b){
			return {std::max(a.min, b.min), std::max(a.avg, b.avg), std::max(a.max, b.max)};
		}
		bool operator !() { return (min == -1) && (avg == -1) && (max == -1); }
		bool operator ==(const answer& a) const { return (min == a.min) && (avg == a.avg) && (max == a.max);}
		bool operator !=(const answer& a) const { return !(*this == a); }
	public:
		value_t min, avg, max;
	};

public:
	solver(const std::string& args):opcode( {0, 1, 2, 3} ){ 
		// TODO: explore the tree and save the result
		board state;
		/*action::place(0, 1).apply(state);
		action::place(3, 1).apply(state);
		std::cout << state << std::endl;
		action::slide(2).apply(state);
		std::cout << state;*/
		std::vector<int> bag;
		bag.push_back(2);bag.push_back(3);
		state.hint(1);
		expectiminimax(state, state_type(state_type::after), bag);
//		std::cout << "feel free to display some messages..." << std::endl;
	}
	answer expectiminimax(board current, state_type t, std::vector<int> bag){
		int valid_num = 0;
		answer a;
		std::cout << t << " " << current << " " << current.hint() << " dir:" << current.last_dir()<< std::endl;
		if (t.is_before()){
			
			if(!!tableB[index(current)][current.hint()]) return tableB[index(current)][current.hint()];
			for (int op : opcode) {
				board after = board(current).slide_with_board(op);
				if (after == board()) continue;
				answer score_a = expectiminimax(after, state_type(state_type::after), bag);
				a = a.maximum(a, score_a);
				valid_num ++;
			}
			if (valid_num == 0) {
				float score = current.get_score(); 
				tableB[index(current)][current.hint()] = { score, score, score };
				return { score, score, score };
			}else{
				return a;
			}
		}else if (t.is_after()){
			std::cout << "hi:" << std::endl ;
		//	if(!!tableA[index(current)][current.hint()][current.last_dir()-1]) return tableA[index(current)][current.hint()][current.last_dir()-1];
			std::array<int, 6> line;
			switch (current.last_dir()){
					case 1: // left
						line = {2, 5, -1, -1, -1, -1};
						break;
					case 2: // right
						line = {0, 3, -1, -1, -1, -1};
						break;
					case 3: // up
						line = {3, 4, 5, -1, -1, -1};
						break;
					case 4: // down
						line = {0, 1, 2, -1, -1, -1};
						break;
					case 0: // place
					default:
						line = {0, 1, 2, 3, 4, 5};
						break;
			}
			float min = 100000, avg = 0, max = 0;
			for (int pos : line) {	
				if (pos == -1 || current(pos) != 0) continue;
				board after(current);
				action::place(pos, current.hint()).apply(after);
				for (size_t i=0 ; i<bag.size() ; i++){
					std::vector<int> after_bag = bag;
					after.hint(after_bag[i]);
					after_bag.erase(after_bag.begin()+i);
					if (after_bag.empty()){
						after_bag.push_back(1);after_bag.push_back(2);after_bag.push_back(3);
					}
					answer score_b = expectiminimax(after, state_type(state_type::before), after_bag);
					if (score_b.min < min) min = score_b.min; 
					if (score_b.max > max) max = score_b.max; 
					avg += score_b.avg;
					valid_num ++;					
				}
			} 
			avg /= valid_num;
			std::cout << min << avg << max;
			tableA[index(current)][current.hint()][current.last_dir()-1] = {min, avg, max};
			return {min, avg, max};
		}else{
			return -1;
		}
	}
	answer solve(const board& state, state_type type = state_type::before) {
		// TODO: find the answer in the lookup table and return it
		//       do NOT recalculate the tree at here

		// to fetch the hint (if type == state_type::after, hint will be 0)
//		board::cell hint = state_hint(state);

		// for a legal state, return its three values.
//		return { min, avg, max };
		// for an illegal state, simply return {}
		return {};
	}
	
	int index(board b){
		int t1 = b(0), t2 = b(1), t3 = b(2), t4 = b(3), t5 = b(4), t6 = b(5);
		return ( t1 + 6*t2 + 36*t3 + pow(6, 3)*t4 + pow(6, 4)*t5 + pow(6, 5)*t6);
	}
private:
	// TODO: place your transposition table here
	// int tableA[tile 0][tile 1] ... [tile 5][hint][last action];
	static answer tableA[46656][3][4];
	static answer tableB[46656][3];
	std::array<int, 4> opcode;
};

solver::answer solver::tableA[46656][3][4];
solver::answer solver::tableB[46656][3];