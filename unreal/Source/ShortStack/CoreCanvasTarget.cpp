#include "CoreCanvasTarget.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/App.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "SlateDrawList.h"

FCoreCanvasTarget::~FCoreCanvasTarget()
{
	Release();
}

UTextureRenderTarget2D* FCoreCanvasTarget::MakeTarget(UObject* Outer, int32 Width, int32 Height)
{
	if (!FApp::CanEverRender())
	{
		return nullptr;
	}
	// As the widget component makes its own: the canvas draws into it through Slate.
	UTextureRenderTarget2D* T = NewObject<UTextureRenderTarget2D>(Outer, NAME_None, RF_Transient);
	T->ClearColor = FLinearColor::Transparent;
	T->InitCustomFormat(FMath::Max(Width, 8), FMath::Max(Height, 8), PF_B8G8R8A8, false);
	T->UpdateResourceImmediate(true);
	return T;
}

void FCoreCanvasTarget::Render(UTextureRenderTarget2D* Target, const TSharedPtr<const ss::ui::DrawList>& List)
{
	if (!Target || !List.IsValid() || !FSlateApplication::IsInitialized() || !FApp::CanEverRender())
	{
		return;
	}
	if (!Renderer)
	{
		Renderer = new FWidgetRenderer(false, true);
	}
	const FVector2D Size(Target->SizeX, Target->SizeY);
	TSharedRef<SDrawListWidget> Widget = SNew(SDrawListWidget).DesiredSize(Size);
	Widget->SetDrawList(List);
	Renderer->DrawWidget(Target, Widget, Size, 0.0f);
}

void FCoreCanvasTarget::Release()
{
	if (Renderer)
	{
		BeginCleanup(Renderer);
		Renderer = nullptr;
	}
}
