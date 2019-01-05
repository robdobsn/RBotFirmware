#include "stdio.h"

#include "EvaluatorThetaRhoLine.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

EvaluatorThetaRhoLine eval;

int main()
{
	ifstream inFile;
	inFile.open("DitherCells.thr");

	if (!inFile) {
		printf("Unable to open file\n");
		exit(1);
	}

	string str;
	int i = 0;
	double theta;
	bool firstLine = true;
	while (inFile >> str)
	{
		//printf("%s\n", str.c_str());
		double val = atof(str.c_str());
		if (i == 1)
		{
			eval.execWorkItem(firstLine, theta, val);
			firstLine = false;

			for (int i = 0; i < 10000; i++)
			{
				if (!eval.isBusy())
					break;
				eval.service();
			}
		}
		else
		{
			theta = val;
		}
		i++;
		if (i > 1)
			i = 0;
	}

	//getchar();
}