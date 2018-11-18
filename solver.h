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
		answer(value_t min = 0.0/0.0, value_t avg = 0.0/0.0, value_t max = 0.0/0.0) : min(min), avg(avg), max(max) {}
	    friend std::ostream& operator <<(std::ostream& out, const answer& ans) {
	    	return !std::isnan(ans.avg) ? (out << ans.min << " " << ans.avg << " " << ans.max) : (out << "-1") << std::endl;
		}
	public:
		const value_t min, avg, max;
	};

public:
	solver(const std::string& args) {
		// TODO: explore the tree and save the result
		board state;
		action::place(0, 1).apply(state);
		action::place(3, 1).apply(state);
		std::cout << state << std::endl;
		action::slide(2).apply(state);
		std::cout << state;
//		std::cout << "feel free to display some messages..." << std::endl;
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
	
	int index(int t1, int t2, int t3, int t4, int t5, int t6){
		return ( t1 + 6*t2 + 36*t3 + pow(6, 3)*t4 + pow(6, 4)*t5 + pow(6, 5)*t6);
	}
private:
	// TODO: place your transposition table here
	// int tableA[type][tile 0][tile 1] ... [tile 5][hint][last action][min];
	//int tableA[2][46656][3][4][3];
	//int tableB[2][46656][3][3];
};
