//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


#include "MultipleViewports.h"
#include <Urho3D/Urho3DAll.h>

URHO3D_DEFINE_APPLICATION_MAIN(MultipleViewports)

MultipleViewports::MultipleViewports(Context* context) :
    Sample(context)
{
}

void MultipleViewports::Setup()
{
	Sample::Setup();
	engineParameters_[EP_LOG_NAME] = "Urho3D.log";
}

void MultipleViewports::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

	CreateUI();
    // Create the UI content
    CreateInstructions();

    // Setup the viewports for displaying the scene
    SetupViewports();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void MultipleViewports::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    // Create scene node & StaticModel component for showing a static plane
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    auto* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create some mushrooms
    const unsigned NUM_MUSHROOMS = 240;
    for (unsigned i = 0; i < NUM_MUSHROOMS; ++i)
    {
        Node* mushroomNode = scene_->CreateChild("Mushroom");
        mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
        mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        mushroomNode->SetScale(0.5f + Random(2.0f));
        auto* mushroomObject = mushroomNode->CreateComponent<StaticModel>();
        mushroomObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
        mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
        mushroomObject->SetCastShadows(true);
    }

    // Create randomly sized boxes. If boxes are big enough, make them occluders
    const unsigned NUM_BOXES = 20;
    for (unsigned i = 0; i < NUM_BOXES; ++i)
    {
        Node* boxNode = scene_->CreateChild("Box");
        float size = 1.0f + Random(10.0f);
        boxNode->SetPosition(Vector3(Random(80.0f) - 40.0f, size * 0.5f, Random(80.0f) - 40.0f));
        boxNode->SetScale(size);
        auto* boxObject = boxNode->CreateComponent<StaticModel>();
        boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        boxObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        boxObject->SetCastShadows(true);
        if (size >= 3.0f)
            boxObject->SetOccluder(true);
    }

    // Create the cameras. Limit far clip distance to match the fog
    cameraNode_ = scene_->CreateChild("Camera");
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the front camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void MultipleViewports::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText(
        "Use WASD keys and hold RMB to move\n"
        "F2 to toggle DebugHud\n"
        "F10 to save screen\n"
    );
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void MultipleViewports::SetupViewports()
{
    auto* graphics = GetSubsystem<Graphics>();
    auto* renderer = GetSubsystem<Renderer>();

    renderer->SetNumViewports(2);

    // Set up the front camera viewport
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);

    auto* cache = GetSubsystem<ResourceCache>();
	SharedPtr<RenderPath> effectRenderPath = MakeShared<RenderPath>();
	effectRenderPath->Load(cache->GetResource<XMLFile>("RenderPaths/myForwardDepth.xml"));
	effectRenderPath->SetEnabled("ao_only", false);
    effectRenderPath->SetEnabled("ssao", false);
	effectRenderPath->SetEnabled("oil_paint", false);
    effectRenderPath->SetEnabled("edge_detect", false);
	effectRenderPath->SetEnabled("posterization", false);
	effectRenderPath->SetEnabled("FXAA3", false);
	effectRenderPath->SetEnabled("film_grain", false);
    viewport->SetRenderPath(effectRenderPath);
}

void MultipleViewports::SubscribeToEvents()
{
    // Subscribe HandleUpdate() method for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(MultipleViewports, HandleUpdate));

	SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(MultipleViewports, HandleKeyDown));
}

void MultipleViewports::MoveCamera(float timeStep)
{
     // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

	if (input->GetMouseButtonDown(MOUSEB_RIGHT))
	{
		// Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
		IntVector2 mouseMove = input->GetMouseMove();
		yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
		pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
		pitch_ = Clamp(pitch_, -90.0f, 90.0f);

		// Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
		cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
	}

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

}

void MultipleViewports::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void MultipleViewports::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
	using namespace KeyDown;

	int key = eventData[P_KEY].GetInt();
	if (key == KEY_F10)
	{
		Graphics * graphic = GetSubsystem<Graphics>();
		SharedPtr<Image> img(MakeShared<Image>(context_));
		if (graphic->TakeScreenShot(*img))
		{
			Time * time = GetSubsystem<Time>();
			String timeStamp(time->GetTimeStamp());
			timeStamp.Replace(':', '.');
			img->SavePNG(String("ScreenShot ") + timeStamp + String(".png"));
		}
	}
	else if (key == KEY_F2)
		GetSubsystem<DebugHud>()->ToggleAll();
}

void MultipleViewports::CreateChkBox(UIElement * parent, const String &s)
{
	UIElement * hLayout = parent->CreateChild<UIElement>();
	hLayout->SetAlignment(HA_LEFT, VA_TOP);
	hLayout->SetLayout(LM_HORIZONTAL, 5, IntRect(5, 5, 5, 5));
	
	CheckBox * cb = hLayout->CreateChild<CheckBox>();
	cb->SetStyleAuto();
	cb->SetVar("PostProcess", s);
	SubscribeToEvent(cb, E_TOGGLED, URHO3D_HANDLER(MultipleViewports, HandlePostProcess));

	auto* cache = GetSubsystem<ResourceCache>();
	Text * caption = hLayout->CreateChild<Text>();
	caption->SetStyleAuto();
	caption->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 12);
	caption->SetTextAlignment(HA_LEFT);
	caption->SetText(s);
}

void MultipleViewports::CreateUI()
{
	auto* cache = GetSubsystem<ResourceCache>();
	auto* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

	UIElement * root = GetSubsystem<UI>()->GetRoot();
	root->SetDefaultStyle(style);

	UIElement * vLayout = root->CreateChild<UIElement>();
	vLayout->SetAlignment(HA_LEFT, VA_TOP);
	vLayout->SetLayout(LM_VERTICAL, 5, IntRect(5, 5, 5, 5));
	CreateChkBox(vLayout, "ao_only");
	CreateChkBox(vLayout, "ssao");
	CreateChkBox(vLayout, "oil_paint");
	CreateChkBox(vLayout, "edge_detect");
	CreateChkBox(vLayout, "posterization");
	CreateChkBox(vLayout, "FXAA3");
	CreateChkBox(vLayout, "film_grain");
}

void MultipleViewports::HandlePostProcess(StringHash eventType, VariantMap& eventData)
{
	using namespace Toggled;

	CheckBox * box = static_cast<CheckBox*>(eventData[P_ELEMENT].GetPtr());
	auto* renderer = GetSubsystem<Renderer>();
	Viewport * vp = renderer->GetViewport(0);
	RenderPath * rp = vp->GetRenderPath();
	rp->SetEnabled(box->GetVar("PostProcess").GetString(), box->IsChecked());
}
