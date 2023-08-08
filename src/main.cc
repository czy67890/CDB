#include <iostream>
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>
#include <limits>
#include <algorithm>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <cstring>
#include <mutex>
#include <bits/stdc++.h>
#include <array>
using namespace std;
template<size_t N>
class UniFond {
public:
	UniFond() {
		for (int i = 0; i < N; ++i) {
			parent_[i] = i;
		}
	}

	void unionNode(int x, int y) {
		int parentX = find(x);
		int parentY = find(y);
		if (parentX == parentY) {
			return;
		}
		parent_[parentX] = parentY;
		return;
	}

	int find(int x) {
		if (x == parent_[x]) {
			return x;
		}
		parent_[x] = find(parent_[x]);
		return parent_[x];
	}

	bool isConnected(int x, int y) {
		return find(x) == find(y);
	}


private:
	std::array<int, N> parent_;
};



class Solution {
public:
	bool equationsPossible(vector<string>& equations) {

		sort(equations.begin(), equations.end(), [](const string& a, const string& b) {
			if(a[1] == '=' && b[1] == '='){
				return a < b;
			}	
			else if(a[1] == '=' && b[1] == '!'){
				return true;
			}
			else if(a[1] == '!' && b[1] == '='){
				return false;
			}
			return false;
		});

		UniFond<26> uni;


		for (const auto& eq : equations) {
			int nodeA = eq[0] - 'a';			int nodeB = eq[3] - 'a';

			if (eq[1] == '!') {
				if (uni.isConnected(nodeA, nodeB)) {
					return false;
				}
			}
			else {
				uni.unionNode(nodeA, nodeB);
			}
		}
		return true;
	}
};


int main() {
	Solution s;
	std::vector<string > vecStr{ "f==d","y!=g","i!=u","b==l","r!=e","s!=d","a!=y","p==e","w!=f","l==c","r==n","o!=h","x==f","g==v","q!=a","l==l","e==p","o!=r","f!=o","n==n" };
	s.equationsPossible(vecStr);
	return 0;
}