#include <iostream>
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>
#include "Util/NoDestructor.h"
#include <limits>
#include <algorithm>
using namespace std;
class Solution {
public:
	string minimumString(string a, string b, string c) {
		vector<string> vec{ a,b,c };
		sort(vec.begin(), vec.end());
		string tmp = merge(vec[0], vec[1]);
		auto res1 = merge(tmp, vec[2]);
		auto res2 = merge(merge(vec[1],vec[2]),vec[0]);
		if(res1.length() == res2.length()){
			return res1 < res2 ? res1: res2;
		}
		return res1.length() < res2.length() ? res1 : res2;
	}

	string merge(const string& a, const string& b) {
		int aLen = a.length();
		int bLen = b.length();
		for (int backLen = min(aLen, bLen); backLen >= 0; --backLen) {
			bool aCan = false;
			bool bCan = false;
			if (a.substr(aLen - backLen) == b.substr(0, backLen)) {
				aCan = true;
			}
			if (b.substr(bLen - backLen) == a.substr(0, backLen)) {
				bCan = true;
			}
			if (aCan && bCan) {
				if (a < b) {
					return mergeHelper(a, b, backLen);
				}
				else {
					return mergeHelper(b, a, backLen);
				}
			}
			if (aCan) {
				return mergeHelper(a, b, backLen);
			}
			if (bCan) {
				return mergeHelper(b, a, backLen);
			}
		}
		return a < b ? a + b : b + a;
	}

	string mergeHelper(const string& a, const string& b, int commonLen) {
		if (commonLen == 0) {
			return a + b;
		}
		return a + b.substr(commonLen);
	}

};
int main(){
	Solution s;
	cout << s.minimumString("a",
		"b"
		,"ba");
	return 0;
}