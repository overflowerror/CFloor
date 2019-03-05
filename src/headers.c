#include <stdlib.h>
#include <string.h>

#include "headers.h"
#include "misc.h"

int headers_find(struct headers* headers, const char* key) {
	for (int i = 0; i < headers->number; i++) {
		if (strcmp(headers->headers[i].key, key) == 0)
			return i;
	}
	return -1;
}

const char* headers_get(struct headers* headers, const char* key) {
	int tmp = headers_find(headers, key);
	if (tmp < 0)
		return NULL;
	return headers->headers[tmp].value;
}

int headers_mod(struct headers* headers, char* key, char* value) {
	int index = headers_find(headers, key);
	
	if (index < 0) {
		struct header* tmp = realloc(headers->headers, (headers->number + 1) * sizeof(struct header));
		if (tmp == NULL) {
			// we don't need to clean up this connection gets dropped anyway
			return HEADERS_ALLOC_ERROR;
		}

		headers->headers = tmp;

		index = headers->number++;
	}

	headers->headers[index] = (struct header) {
		.key = key,
		.value = value
	};

	return index;
}

int headers_parse(struct headers* headers, const char* _currentHeader, size_t length) {
	char* key = NULL;
	char* value = NULL;

	if (length == 0) {
		return HEADERS_END;
	}

	char* currentHeader = malloc(length);
	if (currentHeader == NULL)
		return HEADERS_ALLOC_ERROR;

	memcpy(currentHeader, _currentHeader, length);
	// string terminator not needed

	char* keyTmp = currentHeader;
	char* valueTmp = NULL;

	for(int i = 0; i < length; i++) {
		if (currentHeader[i] == ':') {
			keyTmp[i] = '\0';
			valueTmp = currentHeader + i + 1;
			length -= i + 1;
			break;
		}
	}
	if (length < 0) {
		free(currentHeader);
		return HEADERS_PARSE_ERROR;
	}
	if (valueTmp == NULL) {
		free(currentHeader);
		return HEADERS_PARSE_ERROR;
	}
	key = malloc(strlen(keyTmp) + 1);
	if (key == NULL) {
		free(currentHeader);
		return HEADERS_ALLOC_ERROR;
	}	
	value = malloc(length + 1);
	if (value == NULL) {
		free(currentHeader);
		return HEADERS_ALLOC_ERROR;
	}
	strcpy(key, keyTmp);
	memcpy(value, valueTmp, length);
	value[length] = '\0';

	free(currentHeader);

	int shift = 0;
	for(int i = 0; i < strlen(value); i++) {
		if (value[i] != ' ') {
			shift = i;
			break;
		}
	}
	if (shift > 0) {
		memmove(value, value + shift, strlen(value) - shift + 1);
	}
	for(int i = strlen(value) - 1; i >= 0; i--) {
		if (value[i] != ' ') {
			value[i + 1] = '\0';
			break;
		}
	}

	return headers_mod(headers, key, value);
}

void headers_free(struct headers* headers) {
	for (int i = 0; i < headers->number; i++) {
		if (headers->headers[i].key != NULL)
			free(headers->headers[i].key);
		if (headers->headers[i].value != NULL)
			free(headers->headers[i].value);
	}
	
	if (headers->headers != NULL)
		free(headers->headers);
}

int headers_metadata(struct metaData* metaData, char* header) {

	char* _method = strtok(header, " ");
	if (_method == NULL)
		return HEADERS_PARSE_ERROR;
	char* _path = strtok(NULL, " ");
	if (_path == NULL)
		return HEADERS_PARSE_ERROR;
	char* _httpVersion = strtok(NULL, " ");
	if (_httpVersion == NULL)
		return HEADERS_PARSE_ERROR;

	char* _null = strtok(NULL, " ");
	if (_null != NULL)
		return HEADERS_PARSE_ERROR;

	_path = strtok(_path, "#");
	int tmp = strlen(_path);
	_path = strtok(_path, "?");
	char* _queryString = "";
	if (tmp > strlen(_path)) {
		_queryString = _path + strlen(_path) + 1;
	}

	enum method method;

	if (strcmp(_method, "GET") == 0)
		method = GET;
	else if (strcmp(_method, "POST") == 0)
		method = POST;
	else if (strcmp(_method, "PUT") == 0)
		method = PUT;
	else
		return HEADERS_PARSE_ERROR;

	enum httpVersion httpVersion;
	if (strcmp(_httpVersion, "HTTP/1.0") == 0)
		httpVersion = HTTP10;
	else if (strcmp(_httpVersion, "HTTP/1.1") == 0)
		httpVersion = HTTP11;
	else
		return HEADERS_PARSE_ERROR;

	char* path = malloc(strlen(_path) + 1);
	if (path == NULL) {
		return HEADERS_ALLOC_ERROR;
	}
	char* queryString = malloc(strlen(_queryString) + 1);
	if (queryString == NULL) {
		free(path);
		return HEADERS_ALLOC_ERROR;
	}
	
	metaData->method = method;
	metaData->httpVersion = httpVersion;
	metaData->path = path;
	metaData->queryString = queryString;

	return HEADERS_SUCCESS;
}
