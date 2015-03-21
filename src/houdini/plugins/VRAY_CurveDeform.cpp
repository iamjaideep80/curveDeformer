#include <fstream>

#include <GU/GU_Detail.h>
#include <GA/GA_Detail.h>
#include <UT/UT_Color.h>
#include "VRAY_CurveDeform.h"

class VRAY_SingleCurveDeform : public VRAY_Procedural
{
public:
	VRAY_SingleCurveDeform(int curveNum, UT_String rootPath, UT_String instanceName, LOD_Config lodConfig) :
			myCurveNum(curveNum), rootPath(rootPath), instanceName(instanceName), lodConfig(lodConfig)
	{
	}
	virtual ~VRAY_SingleCurveDeform()
	{
	}

	virtual const char *getClassName();
	virtual int initialize(const UT_BoundingBox *);
	virtual void getBoundingBox(UT_BoundingBox &box);
	virtual void render();
private:
	int myCurveNum;
	UT_String rootPath;
	UT_String instanceName;
	LOD_Config lodConfig;
};

static VRAY_ProceduralArg theArgs[] =
{
	VRAY_ProceduralArg("minbound", "real", "-1 -1 -1"),
	VRAY_ProceduralArg("maxbound", "real", "1 1 1"),
	VRAY_ProceduralArg("size", "real", ".1"),
	VRAY_ProceduralArg("npoints", "int", "10"),
	VRAY_ProceduralArg("seed", "int", "1"),
	VRAY_ProceduralArg("rootPath", "string", ""),
	VRAY_ProceduralArg("instanceName", "string", ""),
	VRAY_ProceduralArg("doLOD", "int", "0"),
	VRAY_ProceduralArg("doLOD", "int", "0"),
	VRAY_ProceduralArg("lodInputMin", "int", "0"),
	VRAY_ProceduralArg("lodInputMax", "int", "0"),
	VRAY_ProceduralArg("lodOutputMin", "int", "0"),
	VRAY_ProceduralArg("lodOutputMax", "int", "0"),
	VRAY_ProceduralArg("analysisMode", "int", "0"),
	VRAY_ProceduralArg() };

VRAY_Procedural *
allocProcedural(const char *)
{
	return new VRAY_CurveDeform();
}
const VRAY_ProceduralArg *
getProceduralArgs(const char *)
{
	return theArgs;
}

VRAY_CurveDeform::VRAY_CurveDeform()
{
	myBox.initBounds(0, 0, 0);
}

const char *
VRAY_CurveDeform::getClassName()
{
	return "VRAY_CurveDeform";
}

int VRAY_CurveDeform::initialize(const UT_BoundingBox *box)
{
	const GU_Detail* rightSide = queryGeometry(queryObject(0), 0);
	rightSide->getBBox(&myBox);
	return 1;
}

void VRAY_CurveDeform::getBoundingBox(UT_BoundingBox &box)
{
	box = myBox;
}

LOD_Config VRAY_CurveDeform::importLodConfig()
{
	int doLOD[1];
	import("doLOD", doLOD, 1);
	int in_min[1];
	import("lodInputMin", in_min, 1);
	int in_max[1];
	import("lodInputMax", in_max, 1);
	int out_min[1];
	import("lodOutputMin", out_min, 1);
	int out_max[1];
	import("lodOutputMax", out_max, 1);
	int analysisMode[1];
	import("analysisMode", analysisMode, 1);
	LOD_Config lodConfig =
	{ doLOD[0], in_min[0], in_max[0], out_min[0], out_max[0], analysisMode[0] };
	return lodConfig;
}

void VRAY_CurveDeform::render()
{
	const GU_Detail* rightSide = queryGeometry(queryObject(0), 0);

	UT_String rootPath;
	import("rootPath", rootPath, 0);
	UT_String instanceName;
	import("instanceName", instanceName, 0);
	LOD_Config lodConfig = importLodConfig();
	for (int i = 0; i < rightSide->primitives().entries(); i++)
	{
		openProceduralObject();
		addProcedural(new VRAY_SingleCurveDeform(i, rootPath, instanceName, lodConfig));
		closeObject();
	}
}
//---------------------------------------------------------------------
const char *
VRAY_SingleCurveDeform::getClassName()
{
	return "vray_SingleCurveDeform";
}

int VRAY_SingleCurveDeform::initialize(const UT_BoundingBox *)
{
	// Since the procedural is generated by the Stamp, this method should never
	// be called.
	fprintf(stderr, "This method should never be called\n");
	return 0;
}

void VRAY_SingleCurveDeform::getBoundingBox(UT_BoundingBox &box)
{
	UT_BoundingBox myBox;

	const char* rootName = queryRootName();
	const GU_Detail* rightSide = allocateGeometry();
	rightSide = queryGeometry(queryObject(rootName), 0);
	const GEO_Primitive* rightCurve = rightSide->primitives()(myCurveNum);
	rightCurve->getBBox(&myBox);
	box = myBox;
}

void VRAY_SingleCurveDeform::render()
{
	float sampleIncreament = 0.001;
	//--------------------START--------------------init Gdp
	GU_Detail* leftSideTmp = allocateGeometry();
	GU_Detail* gdp = allocateGeometry();
	GU_Detail* leftSide = allocateGeometry();
	const char* rootName = queryRootName();
	const GU_Detail* rightSide = allocateGeometry();
	rightSide = queryGeometry(queryObject(rootName), 0);
	gdp->clearAndDestroy();
	//----------------------END----------------------init Gdp
	const GEO_Primitive* rightCurve = rightSide->primitives()(myCurveNum);
	GA_ROAttributeRef instanceNumAttrRef = rightSide->findPrimitiveAttribute("instanceNum");
	int instanceNum = 0;
	if (instanceNumAttrRef.isValid())
		rightCurve->get(instanceNumAttrRef, instanceNum, 0);
	//--------------------START--------------------calc LOD
	UT_BoundingBox curveBox;
	rightCurve->getBBox(&curveBox);
	float lod = getLevelOfDetail(curveBox);
//	cout << "lod : " << lod << endl;
	std::stringstream fileNameStream;

	float lodColorNum = 0;
	float lodNum;
	int loadNumInt;
	if (lodConfig.doLOD)
	{
		lod = std::max((float) lodConfig.in_min, std::min(lod, (float) lodConfig.in_max));
		float in_diff = (lodConfig.in_max - lodConfig.in_min);
		float out_diff = (lodConfig.out_max - lodConfig.out_min);
		lodNum = ((lod - lodConfig.in_min) * out_diff) / in_diff + lodConfig.out_min;
		lodColorNum = ((lod - lodConfig.in_min) * 1) / in_diff;

//		cout << "lod : " << lod << endl;
//		cout << "lodNum : " << ceil(lodNum) << endl;
		loadNumInt = ceil(lodNum);
//		cout << "loadNumInt : " << loadNumInt << endl;
		fileNameStream << rootPath << "/" << instanceName << "_" << instanceNum << "_lod_" << loadNumInt
				<< ".bgeo";
	}
	else
	{
		fileNameStream << rootPath << "/" << instanceName << "_" << instanceNum << ".bgeo";
	}
	//----------------------END----------------------calc LOD
	GA_Detail::IOStatus status = leftSide->load(fileNameStream.str().c_str(), 0);
	if (!status.success())
	{
		cout << "Instance File Not Found !!!" << endl;
		cout << fileNameStream.str() << endl;
		return;
	}
	//--------------------START--------------------create Atribs
	GA_ROAttributeRef CdAttrRef = rightSide->findPrimitiveAttribute("Cd");
	GA_RWAttributeRef targetCdAttrRef = leftSide->addFloatTuple(GA_ATTRIB_PRIMITIVE, "Cd", 3);
	GEO_Primitive* prim;
	float srcCol[3];
	if (lodConfig.analysisMode == 0)
	{
		if (CdAttrRef.isValid())
		{
			rightCurve->get(CdAttrRef, srcCol, 3);
		}
	}
	if (lodConfig.analysisMode == 1)
	{
		UT_Color color;
		color.setHSV((1 - lodColorNum) * 240, 1, 1);
		color.getRGB(srcCol, srcCol + 1, srcCol + 2);
	}
	if (lodConfig.analysisMode == 2)
	{
//		cout << "lod : " << lod << endl;
//		cout << "loadNumInt : " << loadNumInt << endl;
		srand(loadNumInt + 5);
		srcCol[0] = (float) rand() / RAND_MAX;
		srcCol[1] = (float) rand() / RAND_MAX;
		srcCol[2] = (float) rand() / RAND_MAX;
//		cout << "Color : " << srcCol[0] << "	" << srcCol[1] << "	" << srcCol[2] << endl;
	}
	if (targetCdAttrRef.isValid())
	{
		GA_FOR_ALL_PRIMITIVES(leftSide,prim)
		{
			prim->set<float>(targetCdAttrRef, srcCol, 3);
		}
	}
	//----------------------END----------------------create Atribs
	//--------------------START--------------------normalize left side
	GU_Detail* leftSideNrm = allocateGeometry();
	leftSideNrm->duplicate(*leftSide);
	UT_BoundingBox bbox;
	leftSideNrm->getBBox(&bbox);
	UT_Matrix4 xform;
	xform.identity();
	xform.scale(1 / bbox.xsize(), 1 / bbox.ysize(), 1 / bbox.zsize(), 1);
	leftSideNrm->transform(xform);
	xform.identity();
	leftSide->getBBox(&bbox);
	xform.translate(-bbox.centerX(), -bbox.centerY() + 0.5, -bbox.centerZ());
	leftSideNrm->transform(xform);
	//----------------------END----------------------normalize left side
	//--------------------START--------------------for each right curve
	GA_ROAttributeRef scaleXAttribRef = rightSide->findPrimitiveAttribute("scaleX");
	GA_ROAttributeRef scaleZAttribRef = rightSide->findPrimitiveAttribute("scaleZ");
	leftSideTmp->merge(*leftSideNrm);
	GEO_Point* leftPpt;
	for (int i = 0; i < leftSideTmp->points().entries(); i++)
	{
		leftPpt = leftSideTmp->points()(i);
		UT_Vector4 origPos = leftPpt->getPos();
		float origPosX = origPos[0];
		float origPosY = origPos[1];
		float origPosZ = origPos[2];
		//--------------------START--------------------calculate pos and normal
		UT_Vector4 finalPos1;
		float sample1 = origPosY;
		rightCurve->evaluatePoint(finalPos1, sample1, 0);

		UT_Vector4 finalPos2;
		float sample2 = origPosY + sampleIncreament;
		int endCorrection = 0;
		if (sample2 > 1)
		{
			sample2 = origPosY - sampleIncreament;
			endCorrection = 1;
		}
		rightCurve->evaluatePoint(finalPos2, sample2, 0);

		UT_Vector3 normal;
		if (endCorrection)
		{
			normal = finalPos1 - finalPos2;
		}
		else
		{
			normal = finalPos2 - finalPos1;
		}
		normal.normalize();
		UT_Vector3 yAxis(1, 0, 0);
		UT_Vector3 newXVec(yAxis);
		newXVec.cross(normal);
		newXVec.normalize();

		UT_Vector3 newZVec(newXVec);
		newZVec.cross(normal);
		newZVec.normalize();

		if (normal == yAxis || normal == -yAxis)
		{
			newXVec = UT_Vector3(1, 0, 0);
			newZVec = UT_Vector3(0, 0, 1);
		}
		//----------------------END----------------------calculate pos and normal
		float scaleXAttribVal = 1;
		if (scaleXAttribRef.isValid())
			rightCurve->get(scaleXAttribRef, scaleXAttribVal, 0);
		float scaleZAttribVal = 1;
		if (scaleZAttribRef.isValid())
			rightCurve->get(scaleZAttribRef, scaleZAttribVal, 0);
		leftPpt->setPos(
				finalPos1 + scaleXAttribVal * origPosX * newXVec + scaleZAttribVal * origPosZ * newZVec);
	}
	gdp->merge(*leftSideTmp);

	leftSideTmp->clearAndDestroy();
	//----------------------END----------------------for each right curve

	openGeometryObject();
	addGeometry(gdp, 0);
	closeObject();
}
