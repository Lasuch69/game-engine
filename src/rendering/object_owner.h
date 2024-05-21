#ifndef OBJECT_OWNER_H
#define OBJECT_OWNER_H

#include <cstdint>
#include <unordered_map>

typedef uint64_t ObjectID;

template <typename T> class ObjectOwner {
private:
	std::unordered_map<ObjectID, T> _map;
	uint64_t _last = 0;

public:
	T &operator[](ObjectID object) {
		return _map[object];
	}

	std::unordered_map<ObjectID, T> &map() {
		return _map;
	}

	uint64_t insert(T value) {
		_last++;
		_map[_last] = value;

		return _last;
	}

	bool has(ObjectID object) const {
		auto iter = _map.find(object);
		return iter != _map.end();
	}

	T get_id_or_else(ObjectID object, T value) {
		if (has(object))
			return _map[object];

		return value;
	}

	uint64_t size() const {
		return _map.size();
	}

	void free(ObjectID object) {
		auto iter = _map.find(object);
		bool isFound = iter != _map.end();

		if (!isFound)
			return;

		// template should have viable destructor
		_map.erase(iter);
	};
};

#endif // !OBJECT_OWNER_H
