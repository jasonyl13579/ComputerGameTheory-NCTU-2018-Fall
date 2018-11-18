#pragma once
#include <array>
#include <iostream>
#include <iomanip>
#include <math.h> 

/**
 * array-based board for threes
 *
 * index (1-d form):
 *  (0)  (1)  (2)  
 *  (3)  (4)  (5)  
 *
 */
class board {
public:
	typedef uint32_t cell;
	typedef std::array<cell, 3> row;
	typedef std::array<row, 3> grid;
	//typedef uint64_t data;
	typedef int reward;
	struct data {    
		int previous_dir; 
		int modify;
		reward rewards;
		int hint;
	};    
public:
	board() : tile(), value({ 0, 1, 2, 3, 6, 12, 24, 48, 96, 192, 384, 768, 1536, 3072, 6144, 12288 }) {
		tile[2][0] = 100;
		tile[2][1] = 100;
		tile[2][2] = 100;
		attr.previous_dir = 0;
		attr.modify = -1;
		attr.rewards = 0;
		attr.hint = 0;
	}
	board(const grid& b, data v) : tile(b), value({ 0, 1, 2, 3, 6, 12, 24, 48, 96, 192, 384, 768, 1536, 3072, 6144, 12288}) {
		attr.previous_dir = v.previous_dir;
		attr.modify = v.modify;
		attr.hint = v.hint;
	}
	board(const board& b) = default;
	board& operator =(const board& b) = default;
	//~board();
	operator grid&() { return tile; }
	operator const grid&() const { return tile; }
	row& operator [](unsigned i) { return tile[i]; }
	const row& operator [](unsigned i) const { return tile[i]; }
	cell& operator ()(unsigned i) { return tile[i / 3][i % 3]; }
	const cell& operator ()(unsigned i) const { return tile[i / 3][i % 3]; }

	int last_dir() const { return attr.previous_dir; }
	void last_dir(int d) { attr.previous_dir = d; }
	int hint() const { return attr.hint; }
	void hint(int h) { attr.hint = h; }
	data info() const { return attr; }
	data info(data dat) { data old = attr; attr = dat; return old; }

public:
	bool operator ==(const board& b) const { return tile == b.tile; }
	bool operator < (const board& b) const { return tile <  b.tile; }
	bool operator !=(const board& b) const { return !(*this == b); }
	bool operator > (const board& b) const { return b < *this; }
	bool operator <=(const board& b) const { return !(b < *this); }
	bool operator >=(const board& b) const { return !(*this < b); }

public:

	/**
	 * place a tile (index value) to the specific position (1-d form index)
	 * return 0 if the action is valid, or -1 if not
	 */
	reward place(unsigned pos, cell tile) {
		if (pos >= 6) return -1;
		if (tile != 1 && tile != 2 && tile != 3) return -1;
		operator()(pos) = tile;
		attr.previous_dir = 0;
		return 0;
	}

	/**
	 * apply an action to the board
	 * return the reward of the action, or -1 if the action is illegal
	 */
	board slide_with_board(unsigned opcode) {
		reward r = -1;
		switch (opcode & 0b11) {
			case 0: r = slide_up(); break;
			case 1: r = slide_right(); break;
			case 2: r = slide_down(); break;
			case 3: r = slide_left(); break;
			default: return {};
		}
		if (r == -1) return {};
		else {
			attr.modify = 1;
			return *this;
		}
	}
	reward slide(unsigned opcode) {
		switch (opcode & 0b11) {
			case 0: return slide_up(); 
			case 1: return slide_right();
			case 2: return slide_down(); 
			case 3: return slide_left();
			default: return -1;
		}
	}
	reward slide_left() {
		attr.previous_dir = 1; 
		board prev = *this;
		reward score = 0;
		for (int r = 0; r < 3; r++) {
			auto& row = tile[r];
			//int hold = 0, merge = 1;
			for (int c = 0; c < 2; c++) { //ignore third block
				if (row[c] == 100 && row[c+1] == 100) break;
				if (row[c] == 100) continue;
				if (row[c] == 0 && row[c+1] != 100) {
					row[c] = row[c+1];
					row[c+1] = 0;
				} else if ((row[c] == 1 && row[c+1] == 2) || (row[c] == 2 && row[c+1] == 1)){
					row[c] = 3;
					row[c+1] = 0;
					score += 3;				
				} else if (row[c] == row[c+1] && row[c] != 1 && row[c] != 2 && row[c] != 100){
					row[c]++;
					score += ((pow(3,row[c]-2)) - 2*(pow(3,row[c]-3)));
					row[c+1] = 0;
				}	
			}
		}
		if (*this != prev){
			attr.rewards = score;
			return score;
		}else return -1;
		return (*this != prev) ? score : -1;
	}
	reward slide_right() {
		reflect_horizontal();
		reward score = slide_left();
		reflect_horizontal();
		attr.previous_dir = 2; 
		return score;
	}
	reward slide_up() {
		rotate_right();
		reward score = slide_right();
		rotate_left();
		attr.previous_dir = 3; 
		return score;
	}
	reward slide_down() {
		rotate_right();
		reward score = slide_left();
		rotate_left();
		attr.previous_dir = 4; 
		return score;
	}

	void transpose() {
		for (int r = 0; r < 3; r++) {
			for (int c = r + 1; c < 3; c++) {
				std::swap(tile[r][c], tile[c][r]);
			}
		}
	}

	void reflect_horizontal() {
		for (int r = 0; r < 3; r++) {
			std::swap(tile[r][0], tile[r][2]);
			//std::swap(tile[r][1], tile[r][2]);
		}
	}

	void reflect_vertical() {
		for (int c = 0; c < 3; c++) {
			std::swap(tile[0][c], tile[2][c]);
			//std::swap(tile[1][c], tile[2][c]);
		}
	}

	/**
	 * rotate the board clockwise by given times
	 */
	void rotate(int r = 1) {
		switch (((r % 4) + 4) % 4) {
		default:
		case 0: break;
		case 1: rotate_right(); break;
		case 2: reverse(); break;
		case 3: rotate_left(); break;
		}
	}

	void rotate_right() { transpose(); reflect_horizontal(); } // clockwise
	void rotate_left() { transpose(); reflect_vertical(); } // counterclockwise
	void reverse() { reflect_horizontal(); reflect_vertical(); }
	void reflect_by_index(int i){
		if (i%2 == 0) reflect_horizontal();
		else reflect_vertical(); 
	}
	
public:
	const int get_value (int idx)const{
		return value[idx];
	}
	int get_index(int idx){
		return tile[idx / 3][idx % 3];
	}
	friend std::ostream& operator <<(std::ostream& out, const board& b) {
		for (int i = 0; i < 6; i++) {
			out << std::setw(std::min(i, 1)) << "" << (b.get_value(b(i)));
		}
		return out;
	}
	friend std::istream& operator >>(std::istream& in, board& b) {
		for (int i = 0; i < 6; i++) {
			while (!std::isdigit(in.peek()) && in.good()) in.ignore(1);
			in >> b(i);
			if (b(i) != 0 && b(i) != 1 && b(i) != 2)
				b(i) = std::log2(b(i)/3)+ 3;
		}
		return in;
	}
private:
	grid tile;
	data attr;
	std::array<int, 16> value;
};
