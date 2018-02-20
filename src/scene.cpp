#include "scene.h"

#include <cmath>
#include <iostream>
#include <limits>

#include <embree3/rtcore_ray.h>

namespace {

const int numPhi = 5;
const int numTheta = 2 * numPhi;

void error_handler(void* /*userPtr*/, const RTCError, const char* str) {
	std::cerr << "Device error: " << str << std::endl;
}

}

Scene::Scene() : m_device(rtcNewDevice(NULL)) {
	rtcSetDeviceErrorFunction(m_device, error_handler, NULL);

	m_scene = rtcNewScene(m_device);

}

Scene::~Scene() {
	rtcReleaseScene(m_scene);

	rtcReleaseDevice(m_device);
}

unsigned Scene::addSphere(const Vec3& p, float r) {
	/* create triangle mesh */
	RTCGeometry geom = rtcNewGeometry(m_device, RTC_GEOMETRY_TYPE_TRIANGLE);

	/* map triangle and vertex buffers */
	Vertex *vertices = (Vertex *)rtcSetNewGeometryBuffer(
	                       geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex),
	                       numTheta * (numPhi + 1));
	Triangle *triangles = (Triangle *)rtcSetNewGeometryBuffer(
	                          geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle),
	                          2 * numTheta * (numPhi - 1));

	/* create sphere */
	int tri = 0;
	const float rcpNumTheta = 1.0f / (float)numTheta;
	const float rcpNumPhi = 1.0f / (float)numPhi;
	for(int phi = 0; phi <= numPhi; phi++) {
		for(int theta = 0; theta < numTheta; theta++) {
			const float phif = phi * float(M_PI) * rcpNumPhi;
			const float thetaf = theta * 2.0f * float(M_PI) * rcpNumTheta;

			Vertex &v = vertices[phi * numTheta + theta];
			v.x = p.x + r * sin(phif) * sin(thetaf);
			v.y = p.y + r * cos(phif);
			v.z = p.z + r * sin(phif) * cos(thetaf);
		}
		if(phi == 0)
			continue;

		for(int theta = 1; theta <= numTheta; theta++) {
			int p00 = (phi - 1) * numTheta + theta - 1;
			int p01 = (phi - 1) * numTheta + theta % numTheta;
			int p10 = phi * numTheta + theta - 1;
			int p11 = phi * numTheta + theta % numTheta;

			if(phi > 1) {
				triangles[tri].v0 = p10;
				triangles[tri].v1 = p01;
				triangles[tri].v2 = p00;
				tri++;
			}

			if(phi < numPhi) {
				triangles[tri].v0 = p11;
				triangles[tri].v1 = p01;
				triangles[tri].v2 = p10;
				tri++;
			}
		}
	}

	rtcCommitGeometry(geom);
	unsigned int geomID = rtcAttachGeometry(m_scene, geom);
	rtcReleaseGeometry(geom);
	return geomID;
}

void Scene::commit() {
	rtcCommitScene(m_scene);
}

Vec3 Scene::renderPixel(float x, float y) {
	RTCIntersectContext context;
	rtcInitIntersectContext(&context);

	Vec3 dir(x, y, 1);
	dir.normalize();

	RTCRayHit rayhit;

	rayhit.ray.org_x = 0;
	rayhit.ray.org_y = 0;
	rayhit.ray.org_z = -4;

	rayhit.ray.tnear = 0;
	rayhit.ray.tfar = std::numeric_limits<float>::infinity();

	rayhit.ray.dir_x = dir.x;
	rayhit.ray.dir_y = dir.y;
	rayhit.ray.dir_z = dir.z;

	rayhit.ray.time = 0;

	rayhit.ray.mask = 0xFFFFFFFF;
	rayhit.ray.id = 0;
	rayhit.ray.flags = 0;

	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

	rtcIntersect1(m_scene, &context, &rayhit);

	Vec3 color{1,1,1};

	if(rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
		color = Vec3{(float)(rayhit.hit.geomID & 1), (float)((rayhit.hit.geomID >> 1) & 1), (float)((rayhit.hit.geomID >> 2) & 1)};

		Vec3 norm(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
		norm.normalize();

		const float d = std::abs(dir.dot(norm));

		color.x *= d;
		color.y *= d;
		color.z *= d;
	}

	return color;
}
