#include <iostream>
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>
#include "Util/NoDestructor.h"
#include <limits>
#include <unistd.h>
#include <algorithm>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <cstring>
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
	int fd{ -1 };
	char buf[6]{ 0 };

	fd = open("/mnt/d/Code/CDB/filenew.txt", O_RDONLY);
	int readed = 0;
	while (true) {
		int count = ::pread(fd, buf, sizeof(buf),readed);
		readed += count;
		std::cout << buf << endl;
		if (count <= 0) {
			break;
		}
		
	}

	/*int fd = open("/mnt/d/Code/CDB/filenew.txt", O_RDONLY);
	printf("%d \n", fd);
	char buf[8] = { 0 };
	int num = 0;
	while ((num = read(fd, buf, sizeof(buf))) > 0)
	{
		printf("读到的字符数为:%d \n", num);
		printf("%s \n", buf);
		memset(buf, 0, sizeof(buf));
	}
	printf("文件读取结束了 \n");*/
	return 0;
}