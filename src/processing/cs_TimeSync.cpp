/*
 * Author: Bart van Vliet
 * Copyright: Crownstone B.V. (https://crownstone.rocks)
 * Date: Jun 15, 2017
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#include "processing/cs_TimeSync.h"
#include <cmath> // Required for sqrt, abs
#include <string.h> // Required for memset, memcpy, memcmp

TimeSync::TimeSync() :
	_nodeListSize(0),
	_numNonOutliers(0),
	_mean(0),
	_std(0)
{

}

TimeSync::~TimeSync() {

}

void TimeSync::init(node_id_t* ownNodeId) {
	//! Add ourself as first node
	updateNodeTime(ownNodeId, 0);
//	memset(&(_nodeList[0].id), 0, sizeof(node_id_t));
//	_nodeList[0].timestampDiff = 0;
//	_nodeList[0].isOutlier = false;
//	_nodeListSize++;
//	_numNonOutliers++;
}

void TimeSync::updateNodeTime(node_id_t* nodeId, int64_t timeDiff) {
	//! Search the id in the list, and update the value when found, add it to the list otherwise
	bool found = false;
	uint8_t i=0;
	for (; i<_nodeListSize; ++i) {
		if (memcmp(&(_nodeList[i].id), nodeId, sizeof(node_id_t)) == 0) {
			found = true;
			break;
		}
	}
	if (found) {
		_nodeList[i].timestampDiff = timeDiff;
	}
	else {
		if (_nodeListSize >= TIME_SYNC_MAX_NODE_LIST_SIZE) {
			return;
		}
		_nodeListSize++;
		_numNonOutliers++;
		memcpy(&(_nodeList[i].id), nodeId, sizeof(node_id_t));
		_nodeList[i].timestampDiff = timeDiff;
	}
}

int64_t TimeSync::syncTime() {
	//! Fresh start
	markAllAsNonOutlier();
	markAllOutliers();

	if (_numNonOutliers < 3) {
		return 0;
	}
	//! If this node is not an outlier, don't adjust the time
	if (!_nodeList[0].isOutlier) {
		return 0;
	}

	//! For now, just return the mean.
	//! TODO: it should return a step towards the mean, taking the std into account etc.
	return _mean;
}

void TimeSync::offsetTime(int64_t adjustment) {
	//! Adjust all time diffs, except our own (so start at i=1)
	for (uint8_t i=1; i<_nodeListSize; ++i) {
		_nodeList[i].timestampDiff -= adjustment;
	}
}


void TimeSync::markAllAsNonOutlier() {
	for (uint8_t i=0; i<_nodeListSize; ++i) {
		_nodeList[i].isOutlier = false;
	}
	_numNonOutliers = _nodeListSize;
}

uint8_t TimeSync::calcMean() {
	_mean = 0;
	uint8_t numNonOutlier = 0;
	double sum = 0;
	for (uint8_t i=0; i<_nodeListSize; ++i) {
		if (!_nodeList[i].isOutlier) {
			sum += _nodeList[i].timestampDiff;
			++numNonOutlier;
		}
	}
	if (numNonOutlier) {
		_mean = sum / numNonOutlier;
	}
	return numNonOutlier;
}

uint8_t TimeSync::calcStd() {
	_std = 0;
	uint8_t numNonOutlier = 0;
	double variance = 0;
	for (uint8_t i=0; i<_nodeListSize; ++i) {
		if (!_nodeList[i].isOutlier) {
			double diff = _nodeList[i].timestampDiff - _mean;
			variance += diff*diff;
			++numNonOutlier;
		}
	}
	if (numNonOutlier > 1) {
		variance /= (numNonOutlier - 1); //! unbiased sample variance: divide by n-1 (bessel's correction)
		_std = std::sqrt(variance);
	}
	return numNonOutlier;
}

bool TimeSync::markLargestOutlier() {
	//! Use Thompson Tau test to determine outliers
	//! Used implementation of "Distributed fault detection in smart spaces based on trust management" by Guclu, Ozcelebi, and Lukkien (2016)

	//! Find nr of nodes that are not marked as outlier
	//! and keep up the largest abs difference with the mean.
	uint8_t numNonOutlier = 0;
	double largestDiff = 0;
	uint8_t largestDiffNodeIndex = 0;
	for (uint8_t i=0; i<_nodeListSize; ++i) {
		if (!_nodeList[i].isOutlier) {
			++numNonOutlier;
			double diff = std::abs(_nodeList[i].timestampDiff - _mean);
			if (diff > largestDiff) {
				largestDiff = diff;
				largestDiffNodeIndex = i;
			}
		}
	}

	//! If there's only 2 non outliers, the distribution isn't meaningful?
	if (numNonOutlier < 3) {
		return false;
	}

	//! Calculate the rejection region (tau)
	float tCrit = _tDistributionCriticalValuesTable[numNonOutlier - 3];
	_printVal = tCrit;
//	float tau = tCrit / (numNonOutlier * (numNonOutlier - 2 + tCrit*tCrit));
	float tau = tCrit * (numNonOutlier - 1) / (std::sqrt(numNonOutlier) * std::sqrt(numNonOutlier - 2 + tCrit*tCrit));
	_tau = tau;

	//! Test if outlier
	if (largestDiff > tau * _std) {
		_nodeList[largestDiffNodeIndex].isOutlier = true; //! Mark as outlier
		return true;
	}
	return false;
}

uint8_t TimeSync::markAllOutliers() {
	uint8_t numOutliersMarked = 0;
	uint8_t numNonOutliers;
	for(;;++numOutliersMarked) {
		numNonOutliers = calcMean();
		if (numNonOutliers < 2) {
			break;
		}
		calcStd();
		if (!markLargestOutlier()) {
			break;
		}
	}
	_numNonOutliers = numNonOutliers;

	return numOutliersMarked;
}
