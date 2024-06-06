#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <rendering/effects/effects.h>
#include <rendering/types/allocated.h>

#include <rendering/rendering_device.h>

class Environment {
private:
	BRDFEffect _brdfEffect;
	IrradianceFilterEffect _irradianceFilterEffect;
	SpecularFilterEffect _specularFilterEffect;

public:
	void create(RenderingDevice &renderingDevice);
};

#endif // !ENVIRONMENT_H
