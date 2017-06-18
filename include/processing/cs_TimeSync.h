/*
 * Author: Bart van Vliet
 * Copyright: Crownstone B.V. (https://crownstone.rocks)
 * Date: Jun 15, 2017
 * License: LGPLv3+, Apache License, or MIT, your choice
 */

#pragma once

#include <stdint.h>
#include "common/cs_Types.h" // Required for cs_ble_gap_addr_t

#define TIME_SYNC_MAX_NODE_LIST_SIZE 25

typedef cs_ble_gap_addr_t node_id_t;

struct node_item_t {
	node_id_t id;
	int64_t timestampDiff;
	bool isOutlier;
};

class TimeSync {
public:
	TimeSync();
	~TimeSync();

	/** Update (or add) the time of a node
	 *
	 * @nodeId pointer to the node id
	 * @timeDiff difference with own timestamp
	 */
	void updateNodeTime(node_id_t* nodeId, int64_t timeDiff);

	/** Calculate the adjustment to make in order to synchronize
	 *
	 * @return the adjustment to make
	 */
	int64_t syncTime();

	/** When own clock is adjusted, all stored timeDiffs should be offset by -1*adjustment
	 *
	 * @adjustment How many seconds own clock was adjusted
	 */
	void offsetTime(int64_t adjustment);

//private:
public:

	//! Marks all nodes as not being an outlier
	void markAllAsNonOutlier();

	/** Calculates the mean of all nodes that are not marked as outlier
	 *
	 * @return number of non outliers
	 */
	uint8_t calcMean();

	/** Calculates the standard deviation of all nodes that are not marked as outlier
	 *
	 * @return number of non outliers
	 */
	uint8_t calcStd();

	/** Marks node that is the largest outlier.
	 *
	 * @return true when outlier was marked.
	 */
	bool markLargestOutlier();

	/** Determines which nodes are outliers and mark them as outlier.
	 *  Also calculates mean and std from remaining nodes.
	 *
	 * @return number of nodes that were marked as outlier
	 */
	uint8_t markAllOutliers();



	node_item_t _nodeList[TIME_SYNC_MAX_NODE_LIST_SIZE];
	uint8_t _nodeListSize;
	uint8_t _numNonOutliers;

	double _mean;
	float _std;
	float _tau;
	float _printVal;

	//! Student's t distribution critical values for alpha = 0.05 / 2
	const float _tDistributionCriticalValuesTable[30] = {
		12.71, // degrees of freedom = 1
		4.303, // degrees of freedom = 2
		3.182, // etc.
		2.776,
		2.571,
		2.447,
		2.365,
		2.306,
		2.262,
		2.228,
		2.201,
		2.179,
		2.160,
		2.145,
		2.131,
		2.120,
		2.110,
		2.101,
		2.093,
		2.086,
		2.080,
		2.074,
		2.069,
		2.064,
		2.060,
		2.056,
		2.052,
		2.048,
		2.045,
		2.042,
	};

};



