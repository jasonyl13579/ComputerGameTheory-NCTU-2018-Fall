#pragma once
#include <string>
#include <vector>
#include "board.h"
#include "weight.h"
#include <fstream>
using namespace std;
class pattern {
public:
	pattern(const std::string& info = "simple"){
		if ( info == "simple"){
			std::array<int, 4> tupleArray = {0, 1, 2, 3}; 
			vector<int> tuple;
			for (int i=0; i<4; i++){
				tupleArray = {0 + 4*i, 1 + 4*i, 2 + 4*i, 3 + 4*i};
				tuple.assign(tupleArray.begin(), tupleArray.end()); 
				tuples.emplace_back(tuple);
			}		
		}
	}
	pattern(const pattern& f) = default;

	pattern& operator =(const pattern& f) = default;
	vector<int> & operator[] (size_t i) { return tuples[i]; }
	const vector<int> & operator[] (size_t i) const { return tuples[i]; }
	size_t size() const { return tuples.size(); }
	
	
private:
	std::vector<vector<int>> tuples;
};