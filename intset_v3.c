/*
 * src/tutorial/intset.c
 *
 ******************************************************************************
 This file contains routines that can be bound to a Postgres backend and
 called by the backend in the process of processing queries.  The calling
 format for these routines is dictated by Postgres architecture.
 ******************************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "stdint.h"
#include "regex.h"
PG_MODULE_MAGIC;

typedef struct IntSet
{
	int32		length; // struct length
	int32		size;	// array size
	// char		nums[FLEXIBLE_ARRAY_MEMBER]; // list of num in string format
	int32		data[FLEXIBLE_ARRAY_MEMBER]; // actual length of the data part is not specified
} IntSet;

// FUNCTION DECLARATIONS

bool is_valid_input(char *str);
int32 get_num_length(int32 num);
int32 *get_data(char *str, int32 *size);
char *to_string(int32 *data, int32 size); 
int32 find_insert_pos(int32 *data, int32 target, int32 size);
bool num_exist(int32 *data, int32 target, int32 size);
int32 *insert_num(int32 *data, int32 size, int32 num, int32 pos);
bool is_subset(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB);
bool is_equal(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB);
int32 *get_intersection(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize);
int32 *get_union(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize);
int32 *get_disjunction(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize);
int32 *get_difference(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize);

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/


PG_FUNCTION_INFO_V1(intset_in);

	Datum
intset_in(PG_FUNCTION_ARGS)
{
	char	*str = PG_GETARG_CSTRING(0);
	char 	*tmp = NULL;
	int32 	size = 0;			// size of array
	int32 	*data = NULL;
	IntSet	*result;			// result of IntSet to be stored
	// char 	*debug;
	tmp = malloc(strlen(str) * sizeof(char));
	strcpy(tmp, str);
	
	// if (!is_valid_input(tmp))
	// 	ereport(ERROR,
	// 		(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
	// 			errmsg("invalid input syntax for type %s: \"%s\"",
	// 				"intset", str)));

	// Get list of numbers
	data = get_data(str, &size);

	// debug = to_string(data, size, strLen);
	// ereport(ERROR,
	// 	(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
	// 		errmsg("size is %d\nstrLen is %d\nstring is %s\n", size, strLen, debug)));

	result = (IntSet *) malloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));

	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));

	
	// free(debug);
	free(data);
	free(tmp);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(intset_out);

	Datum
intset_out(PG_FUNCTION_ARGS)
{
	IntSet    *intSet = (IntSet *) PG_GETARG_POINTER(0);
	char	  *result;
	// result = to_string(intSet->data, intSet->size);
	result = to_string(intSet->data, intSet->size);
	//	result = psprintf("{%d}", intSet->data[0]);
	//	result = psprintf("{%d,%d}", intSet->length, intSet->size);
	PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * New Operators
 *
 * A practical intset datatype would provide much more than this, of course.
 *****************************************************************************/

PG_FUNCTION_INFO_V1(intset_contains);

Datum
intset_contains(PG_FUNCTION_ARGS)
{
	int32	  num = PG_GETARG_INT32(0);
	IntSet    *intSet = (IntSet *) PG_GETARG_POINTER(1);
	int32	  *data = intSet->data;
	int32 	  size = intSet->size;
	bool 	  result = num_exist(data, num, size);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(get_cardinality);

Datum
get_cardinality(PG_FUNCTION_ARGS)
{
	IntSet	  *intSet = (IntSet *) PG_GETARG_POINTER(0);
	int32	  result = intSet->size;

	PG_RETURN_INT32(result);
}

PG_FUNCTION_INFO_V1(contains_all);

Datum
contains_all(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);

	bool 	  result = is_subset(setA->data, setA->size, setB->data, setB->size);
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(contains_only);

Datum
contains_only(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);

	bool 	  result = is_subset(setB->data, setB->size, setA->data, setA->size);
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(equal);

Datum
equal(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);

	bool 	  result = is_equal(setB->data, setB->size, setA->data, setA->size);
	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(not_equal);

Datum
not_equal(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);

	bool 	  result = is_equal(setB->data, setB->size, setA->data, setA->size);
	PG_RETURN_BOOL(!result);
}


PG_FUNCTION_INFO_V1(intersection);

Datum
intersection(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);
	int32     size = 0;
	int32 	  *data;
	IntSet	  *result;

	data = get_intersection(setB->data, setB->size, setA->data, setA->size, &size);

	result = (IntSet *) malloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));
	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));
	free(data);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(union_set);

Datum
union_set(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);
	int32     size = 0;
	int32 	  *data;
	IntSet	  *result;

	data = get_union(setA->data, setA->size, setB->data, setB->size, &size);

	result = (IntSet *) malloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));
	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));
	free(data);
	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(disjunction);

Datum
disjunction(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);
	int32     size = 0;
	int32 	  *data;
	IntSet	  *result;

	data = get_disjunction(setA->data, setA->size, setB->data, setB->size, &size);

	result = (IntSet *) malloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));
	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));
	free(data);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(difference);

Datum
difference(PG_FUNCTION_ARGS)
{
	IntSet	  *setA = (IntSet *) PG_GETARG_POINTER(0);
	IntSet	  *setB = (IntSet *) PG_GETARG_POINTER(1);
	int32     size = 0;
	int32 	  *data;
	IntSet	  *result;

	data = get_difference(setA->data, setA->size, setB->data, setB->size, &size);

	result = (IntSet *) malloc((size + 2) * sizeof(int32));
	SET_VARSIZE(result, (size + 2) * sizeof(int32));
	result->size = size;
	memcpy(result->data, data, size * sizeof(int32));
	free(data);
	PG_RETURN_POINTER(result);
}


/*****************************************************************************
 * Helper functions
 *****************************************************************************/

/**
 * Check if the input is valid
 * @param string
 * @return boolean
 */
bool is_valid_input(char *str) {

	
	
	int i = 0, j = strlen(str) - 1, s = 0, l = 0, r = 0;
	char *numList, *token, *tmpStr;
	// ereport(ERROR,
	// 	(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
	// 		errmsg("tmpList %s", str)));
	// remove leading and tailing space
	while (isspace(str[i])) i++;
	while (isspace(str[j])) j--;
	if (str[i] != '{' || str[j] != '}') return false;
	// remove brackets
	i++;
	j--;
	numList = malloc((j - i + 2) * sizeof(char));
	memcpy(numList, &str[i], (j - i + 1));
	// numList[(j - i + 1)] = '\0';
	ereport(ERROR,
		(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
			errmsg("tmpList %s", numList)));
	// deal with non-number characters
	token = strtok(numList, ",");
	while (token != NULL) {
		// remove leading and tailing space
		l = 0, r = strlen(token) - 1;
		while (isspace(token[l])) l++;
		while (isspace(token[r])) r--;

		for (int x = l; x <= r; x++) {
			if (!isdigit(token[x])) {
				// ereport(ERROR,
				// 	(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				// 		errmsg("token --%c--", token[x])));
				return false;
			}
		}
		token = strtok(NULL, ",");
	}
	
	// remove internal spaces
	for (int x = 0; x < strlen(str); x++) {
		if (str[x] != ' ') str[s++] = str[x];
	}

	tmpStr = malloc(sizeof(char) * (s + 1));
	memcpy(tmpStr, &str[0], s);
	tmpStr[s] = '\0';
	
	// deal with special case
	if (strstr(str, ",,")) return false;
	if (strcmp(str, "{}") == 0) return true;
	// ereport(ERROR,
	// 		(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
	// 			errmsg("str: %s\n", str)));
	if (!isdigit(str[1]) || !isdigit(str[s - 2])) {
		// ereport(ERROR,
		// 	(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
		// 		errmsg("0 --%c--\n last: --%c--\n", str[1], str[s-2])));
		return false;
	}
	free(numList);
	free(token);
	free(tmpStr);
	return true;
}


int32 *get_data(char *str, int32 *size) {

	int32 i = 0, j = strlen(str) - 1, n = 0, subLen = 0, subTokenLen = 0, num = 0, tmpSize = 0, *data = NULL, pos = 0;
	char *numsWithZero = NULL, *subStr = NULL, *token = NULL, *subToken = NULL;
	// Remove leading and tailing spaces
	while (isspace(str[i])) i++;
	while (isspace(str[j])) j--;

	// Remove brackets
	i++;
	j--;
	subLen = j - i + 1;
	subStr = malloc((subLen + 1) * sizeof(char));
	memcpy(subStr, &str[i], subLen);
	subStr[subLen] = '\0';

	// Remove internal spaces
	for (i = 0; i < subLen; i++) {
		if (subStr[i] != ' ')
			subStr[n++] = subStr[i];
	}
	numsWithZero = malloc((n + 1) * sizeof(char));
	memcpy(numsWithZero, &subStr[0], n);
	numsWithZero[n] = '\0';

	// Remove leading zeros and fill data array
	token = strtok(numsWithZero, ",");
	while (token != NULL) {
		// i = 0;
		// while (token[i] == '0') i++;
		// if (i == strlen(token)) subTokenLen = 1;
		// else subTokenLen = strlen(token) - i;
		// subToken = malloc(sizeof(char) * subTokenLen);
		// memcpy(subToken, &token[i], subTokenLen);
		i = 0;
		while (token[i] == '0') i++;
		if (i >= strlen(token)) subTokenLen = 1;
		else subTokenLen = strlen(token) - i;
		subToken = malloc(sizeof(char) * (subTokenLen + 1));
		memcpy(subToken, &token[i], subTokenLen);
		subToken[subTokenLen] = '\0';
		// subTokenLen--;
		
		num = atoi(subToken);
		// ereport(ERROR,
		// 	(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
		// 		errmsg("first num: %d\n", subTokenLen)));
		// free(subToken);

		if (tmpSize == 0) {	// add first num to array
			data = malloc(sizeof(int32));
			data[tmpSize++] = num;
		} else {
			// if num already exist in array, jump to next turn
			if (num_exist(data, num, tmpSize)) {
				token = strtok(NULL, ",");
				continue;
			}
			// otherwise find expected position to insert
			
			pos = find_insert_pos(data, num, tmpSize);
			data = insert_num(data, ++tmpSize, num, pos);
		}
		// numsLen += subTokenLen + 1; // 1 stand for comma
		// data = tmpData;	
		// ereport(ERROR,
		// 	(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
		// 		errmsg("subtoken %d\n", subTokenLen)));
		token = strtok(NULL, ",");
	}
	

	*size = tmpSize;
	
	free(token);
	free(subToken);
	free(subStr);
	free(numsWithZero);
	
	return data;
}

bool num_exist(int32 *data, int32 target, int32 size) {
	int32 l = 0, r = size - 1, m;

	while (l <= r) {
		m = l + (r - l) / 2;
		if (data[m] == target) return true;
		else if (data[m] < target) l = m + 1;
		else r = m - 1;
	}
	return false;
}

int32 find_insert_pos(int32 *data, int32 target, int32 size) {
	int32 l = 0, r = size - 1, m;

	while (l <= r) {
		m = l + (r - l) / 2;
		if (data[m] < target) l = m + 1;
		else r = m - 1;
	}
	return l;
}


int32 *insert_num(int32 *data, int32 size, int32 num, int32 pos) {
	
	// int32 *newData;
	data = realloc(data, sizeof(int32) * size);

	for (int32 i = size - 1; i > pos; i--) {
		data[i] = data[i - 1];
	}
	data[pos] = num;
	return data;
}


char *to_string(int32 *data, int32 size) {
	char *str = NULL;
	int32 len;
	if (size == 0) len = 2;
	else len = size + 1; // initialize with number of commas and bracket
	// get string length
	for (int i = 0; i < size; i++) {
		len += get_num_length(data[i]);
	}
	// ereport(ERROR,
	// 		(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
	// 			errmsg("size: %d\nlen: %d", size, len)));

	str = malloc(sizeof(char) * (len + 1));
	
	strcpy(str, "{");
	for (int i = 0; i < size; i++) {
		sprintf(str, "%s%d,", str, data[i]);
	}
	// memcpy(str, &)
	str[len - 1] = '}';
	str[len] = '\0';
	return str;
}

/**
 * Check if intSet A contain all the values in intSet B
 * for every element of B, it is an element of A
 * i.e. A >@ B
 * 
 * @return bool
 */
bool is_subset(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB) {
	for (int i = 0; i < sizeB; i++) {
		// do binary search
		if (!num_exist(dataA, dataB[i], sizeA)) return false;
	}
	return true;
}

bool is_equal(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB) {
	if (sizeA != sizeB) return false;
	for (int i = 0; i < sizeA; i++) {
		if (dataA[i] != dataB[i]) return false;
	}
	return true;
}

int32 get_num_length(int32 num) {
	int32 count = 0;
	if (num == 0) return 1;
	
	while (num != 0) {
		num /= 10;
		count++;
	}
	return count;
}

int32 *get_intersection(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize) {
	int32 *intersection = NULL, size = 0;//, strLen = 0;

	for (int i = 0; i < sizeA; i++) {
		if (!num_exist(dataB, dataA[i], sizeB)) continue;
		if (size == 0) {
			intersection = malloc(sizeof(int32));
			intersection[size++] = dataA[i];
		} else {
			size++;
			intersection = insert_num(intersection, size, dataA[i], size - 1);
		}
	}
	*newSize = size;
	//*newStrLen = strLen;
	return intersection;
}

int32 *get_union(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize) {
	int32 size = sizeB, *unionSet, pos;
	unionSet = malloc(sizeof(int32) * size);
	for (int j = 0; j < sizeB; j++) {
		unionSet[j] = dataB[j];
	}

	for (int i = 0; i < sizeA; i++) {
		if (num_exist(unionSet, dataA[i], size)) continue;
		pos = find_insert_pos(unionSet, dataA[i], size);
		unionSet = insert_num(unionSet, ++size, dataA[i], pos);
	}

	*newSize = size;
	return unionSet;
}

int32 *get_disjunction(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize) {
	int32 interSize = 0, unionSize = 0, size = 0;
	int32 *interSet, *unionSet, *disSet = NULL;
	// char *uni;
	interSet = get_intersection(dataA, sizeA, dataB, sizeB, &interSize);
	
	unionSet = get_union(dataA, sizeA, dataB, sizeB, &unionSize);
	// uni = to_string(unionSet, unionSize);
	// ereport(ERROR,
	// 			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
	// 				errmsg("union: %s\n", uni)));

	for(int i = 0; i < unionSize; i++) {
		if (num_exist(interSet, unionSet[i], interSize)) continue;
		if (size == 0) {
			disSet = malloc(sizeof(int32));
			disSet[size++] = unionSet[i];
		} else {
			size++;
			disSet = insert_num(disSet, size, unionSet[i], size - 1);
		}
	}
	*newSize = size;
	free(interSet);
	free(unionSet);
	return disSet;
}

int32 *get_difference(int32 *dataA, int32 sizeA, int32 *dataB, int32 sizeB, int32 *newSize) {
	int32 size = 0, *diffSet = NULL;

	for(int i = 0; i < sizeA; i++) {
		if (num_exist(dataB, dataA[i], sizeB)) continue;
		if (size == 0) {
			diffSet = malloc(sizeof(int32));
			diffSet[size++] = dataA[i];
		} else {
			size++;
			diffSet = insert_num(diffSet, size, dataA[i], size - 1);
		}
	}
	*newSize = size;
	return diffSet;
}

