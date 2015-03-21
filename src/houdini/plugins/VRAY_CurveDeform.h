#ifndef __VRAY_DemoStamp__
#define __VRAY_DemoStamp__

#include <UT/UT_BoundingBox.h>
#include <VRAY/VRAY_Procedural.h>

struct LOD_Config
{
	int doLOD;
	int in_min, in_max;
	int out_min, out_max;
	int analysisMode;
};

class VRAY_CurveDeform : public VRAY_Procedural
{
public:
	VRAY_CurveDeform();
	virtual ~VRAY_CurveDeform()
	{
	}
	;
	virtual const char *getClassName();
	virtual int initialize(const UT_BoundingBox *);
	virtual void getBoundingBox(UT_BoundingBox &box);
	virtual void render();
private:
	UT_BoundingBox myBox;		// Bounding box
	LOD_Config importLodConfig();
};
#endif
