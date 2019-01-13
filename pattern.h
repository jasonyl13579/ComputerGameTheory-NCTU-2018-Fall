#pragma once
#include <string>
#include <vector>
#include "board.h"
#include "weight.h"
#include <fstream>
using namespace std;
class pattern {
public:
	pattern(const std::string& info = "simple") : type(1){
		if ( info == "simple"){
			type = 1;
			std::array<int, 4> tupleArray = {0, 1, 2, 3}; 
			vector<int> tuple;
			for (int i=0; i<2; i++){
				tupleArray = {0 + 4*i, 1 + 4*i, 2 + 4*i, 3 + 4*i};
				tuple.assign(tupleArray.begin(), tupleArray.end()); 
				tuples.emplace_back(tuple);
			}		
		}
		if ( info == "brute"){
			type = 0;
			std::array<int, 4> tupleArray = {0, 1, 2, 3}; 
			vector<int> tuple;
			for (int i=0; i<4; i++){
				tupleArray = {0 + 4*i, 1 + 4*i, 2 + 4*i, 3 + 4*i};
				tuple.assign(tupleArray.begin(), tupleArray.end()); 
				tuples.emplace_back(tuple);
			}
			for (int i=0; i<4; i++){
				tupleArray = {0 + 4*i, 4 + 4*i, 8 + 4*i, 12 + 4*i};
				tuple.assign(tupleArray.begin(), tupleArray.end()); 
				tuples.emplace_back(tuple);
			}
		}
		if ( info == "enhance" || info == "enhance_hint"){
			type = 2;
			std::array<int, 6> tupleArray = {0, 1, 2, 3, 4, 5}; 
			vector<int> tuple;
			tuple.assign(tupleArray.begin(), tupleArray.end()); 
			tuples.emplace_back(tuple);
			tupleArray = {4, 5, 6, 7, 8, 9};
			tuple.assign(tupleArray.begin(), tupleArray.end()); 
			tuples.emplace_back(tuple);
			tupleArray = {5, 6, 7, 9, 10, 11};
			tuple.assign(tupleArray.begin(), tupleArray.end()); 
			tuples.emplace_back(tuple);
			tupleArray = {9, 10, 11, 13, 14, 15};
			tuple.assign(tupleArray.begin(), tupleArray.end()); 
			tuples.emplace_back(tuple);
		}
	}
	pattern(const pattern& f) = default;

	pattern& operator =(const pattern& f) = default;
	vector<int> & operator[] (size_t i) { return tuples[i]; }
	const vector<int> & operator[] (size_t i) const { return tuples[i]; }
	size_t size() const { return tuples.size(); }
	int get_type(){return type;}
	
private:
	std::vector<vector<int>> tuples;
	int type; // 1 for rotate // 2 for reflect and rotate
};
