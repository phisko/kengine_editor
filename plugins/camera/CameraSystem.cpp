#include "Export.hpp"
#include "EntityManager.hpp"
#include "vector.hpp"
#include "EntityCreator.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/CameraComponent.hpp"
#include "data/ViewportComponent.hpp"
#include "data/InputComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/LightComponent.hpp"

#include "functions/Execute.hpp"

#include "functions/OnMouseCaptured.hpp"
#include "helpers/cameraHelper.hpp"
#include "helpers/pluginHelper.hpp"

#include "imgui.h"
#include "GLFW/glfw3.h"
#include "magic_enum.hpp"
#include "angle.hpp"

#pragma region Adjustables
static auto ROTATION_SPEED = .005f;
static auto MOVEMENT_SPEED = .005f;
static auto ZOOM_SPEED = .1f;
#pragma endregion Adjustables

static kengine::EntityManager * g_em;
static kengine::Entity::ID g_capturedCamera = kengine::Entity::INVALID_ID;
static int g_activeButton = -1;
static kengine::cameraHelper::Facings g_facings;
static putils::Point3f g_lookAt;

#pragma region declarations
static kengine::InputComponent CameraController();
#pragma endregion
EXPORT void loadKenginePlugin(kengine::EntityManager & em) {
	kengine::pluginHelper::initPlugin(em);

	g_em = &em;

	em += [&](kengine::Entity & e) {
		e += kengine::CameraComponent{ { 0.f, 0.f, -1.f } };
		e += kengine::ViewportComponent{};

		e += CameraController();

		e += kengine::AdjustableComponent{
			"Camera", {
				{ "Rotation speed", &ROTATION_SPEED },
				{ "Movement speed", &MOVEMENT_SPEED },
				{ "Zoom speed", &ZOOM_SPEED }
			}
		};
	};
}

#pragma region CameraController
#pragma region declarations
static void processMouseMovement(const putils::Point2f & movement);
static void processMouseScroll(kengine::Entity::ID window, const putils::Point2f & coords, float yoffset);
static void toggleMouseCapture(kengine::Entity::ID window, const putils::Point2f & coords);
#pragma endregion
static kengine::InputComponent CameraController() {
	kengine::InputComponent input;

	input.onMouseMove = [&](kengine::Entity::ID window, const putils::Point2f & coords, const putils::Point2f & rel) {
		if (g_capturedCamera != kengine::Entity::INVALID_ID)
			processMouseMovement(rel);
	};
	input.onScroll = [&](kengine::Entity::ID window, float deltaX, float deltaY, const putils::Point2f & coords) {
		processMouseScroll(window, coords, deltaY);
	};
	input.onMouseButton = [&](kengine::Entity::ID window, int button, const putils::Point2f & coords, bool pressed) {
		g_activeButton = button;
		toggleMouseCapture(window, coords);
	};

	return input;
}

#pragma region processMouseMovement
#pragma region declarations
static void rotateCamera(const putils::Point2f & movement);
static void moveCamera(const putils::Point2f & movement);
static void updateFacingsAndPosition();
#pragma endregion
static void processMouseMovement(const putils::Point2f & movement) {
	switch (g_activeButton) {
	case GLFW_MOUSE_BUTTON_LEFT:
	default:
		rotateCamera(movement);
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		moveCamera(movement);
		break;
	}
}

static void rotateCamera(const putils::Point2f & movement) {
	// First rotate camera
	auto & e = g_em->getEntity(g_capturedCamera);
	auto & cam = e.get<kengine::CameraComponent>();

	cam.yaw -= movement.x * ROTATION_SPEED;
	cam.pitch -= movement.y * ROTATION_SPEED;

	const auto pitchLimit = putils::pi / 2.f - .001f;
	cam.pitch = std::min(cam.pitch, pitchLimit);
	cam.pitch = std::max(cam.pitch, -pitchLimit);

	updateFacingsAndPosition();
}

static void moveCamera(const putils::Point2f & movement) {
	g_lookAt += movement.x * g_facings.right * MOVEMENT_SPEED;
	g_lookAt -= movement.y * g_facings.up * MOVEMENT_SPEED;

	updateFacingsAndPosition();
}

#include "data/DebugGraphicsComponent.hpp"
static void updateFacingsAndPosition() {
	auto & e = g_em->getEntity(g_capturedCamera);
	auto & cam = e.get<kengine::CameraComponent>();

	g_facings = kengine::cameraHelper::getFacings(cam);

	// Then update position (we start from the origin and move backwards)
	const auto dist = (cam.frustum.position - g_lookAt).getLength();
	cam.frustum.position = g_lookAt - g_facings.front * dist;
}
#pragma endregion processMouseMovement

static void processMouseScroll(kengine::Entity::ID window, const putils::Point2f & coords, float yoffset) {
	const auto info = kengine::cameraHelper::getViewportForPixel(*g_em, window, coords);
	if (info.camera == kengine::Entity::INVALID_ID)
		return;

	auto & e = g_em->getEntity(info.camera);
	auto & cam = e.get<kengine::CameraComponent>();

	cam.frustum.position += yoffset * ZOOM_SPEED * g_facings.front;
}

static void toggleMouseCapture(kengine::Entity::ID window, const putils::Point2f & coords) {
	if (g_capturedCamera != kengine::Entity::INVALID_ID)
		g_capturedCamera = kengine::Entity::INVALID_ID;
	else {
		const auto info = kengine::cameraHelper::getViewportForPixel(*g_em, window, coords);
		g_capturedCamera = info.camera;
		g_facings = kengine::cameraHelper::getFacings(g_em->getEntity(g_capturedCamera).get<kengine::CameraComponent>());
	}

	for (const auto & [e, func] : g_em->getEntities<kengine::functions::OnMouseCaptured>())
		func(window, g_capturedCamera != kengine::Entity::INVALID_ID);
}
#pragma endregion CameraController