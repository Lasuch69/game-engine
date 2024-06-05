#ifndef MESH_H
#define MESH_H

#include <cstdint>
#include <rendering/types/vertex.h>

typedef struct {
	Vertex *pData;
	uint32_t count;
} VertexArray;

typedef struct {
	uint32_t *pData;
	uint32_t count;
} IndexArray;

typedef struct {
	VertexArray vertices;
	IndexArray indices;
	uint64_t materialIndex;
} Primitive;

typedef struct {
	Primitive *pPrimitives;
	uint32_t primitiveCount;

	const char *pName;
} Mesh;

#endif // !MESH_H
