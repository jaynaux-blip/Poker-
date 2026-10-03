#include "SlateDrawList.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "HAL/IConsoleManager.h"
#include "Styling/CoreStyle.h"

static TAutoConsoleVariable<int32> CVarTextCache(TEXT("ss.TextCache"), 1, TEXT("1: remember text widths the UI has measured (the pages measure the same strings every frame)."));

// ------------------------------------------------------------------ text measurement

bool FSlateTextMeasurer::IsAvailable()
{
	return FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer() != nullptr;
}

FSlateFontInfo FSlateTextMeasurer::FontFor(ss::ui::Font Face, float SizePx)
{
	// Typefaces of Unreal's default font (Roboto, and Droid Sans Mono for "Mono").
	const TCHAR* Typeface = TEXT("Regular");
	switch (Face)
	{
	case ss::ui::Font::Bold: Typeface = TEXT("Bold"); break;
	case ss::ui::Font::Mono: Typeface = TEXT("Mono"); break;
	case ss::ui::Font::Black: Typeface = TEXT("Black"); break;
	case ss::ui::Font::Light: Typeface = TEXT("Light"); break;
	default: break;
	}
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(FName(Typeface), 10);
	Font.Size = SizePx * 0.75f;
	return Font;
}

const FSlateFontInfo& FSlateTextMeasurer::CachedFont(ss::ui::Font Face, float SizePx) const
{
	const uint64 Key = (static_cast<uint64>(Face) << 32) | static_cast<uint32>(FMath::RoundToInt(SizePx * 64.0f));
	if (const FSlateFontInfo* Found = Fonts.Find(Key))
	{
		return *Found;
	}
	return Fonts.Add(Key, FontFor(Face, SizePx));
}

float FSlateTextMeasurer::Width(const std::string& Text, ss::ui::Font Face, float SizePx) const
{
	if (Text.empty() || !IsAvailable())
	{
		return static_cast<float>(Text.size()) * SizePx * 0.55f;
	}
	const bool bCache = CVarTextCache.GetValueOnAnyThread() != 0;
	FKey Key;
	if (bCache)
	{
		Key.Text = Text;
		Key.Face = static_cast<int32>(Face);
		Key.Size = FMath::RoundToInt(SizePx * 64.0f);
		const auto Found = Widths.find(Key);
		if (Found != Widths.end())
		{
			return Found->second;
		}
	}
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FString Str = FString(UTF8_TO_TCHAR(Text.c_str()));
	const float W = static_cast<float>(Measure->Measure(Str, bCache ? CachedFont(Face, SizePx) : FontFor(Face, SizePx)).X);
	if (bCache)
	{
		if (Widths.size() > 20000)
		{
			Widths.clear();
		}
		Widths.emplace(MoveTemp(Key), W);
	}
	return W;
}

float FSlateTextMeasurer::Ascent(ss::ui::Font Face, float SizePx) const
{
	if (!IsAvailable())
	{
		return SizePx * 0.93f;
	}
	const uint64 AKey = (static_cast<uint64>(Face) << 32) | static_cast<uint32>(FMath::RoundToInt(SizePx * 64.0f));
	if (const float* Found = Ascents.Find(AKey))
	{
		return *Found;
	}
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo& Font = CachedFont(Face, SizePx);
	// The baseline is reported as a negative offset from the bottom of the line.
	return Ascents.Add(AKey, static_cast<float>(Measure->GetMaxCharacterHeight(Font)) + static_cast<float>(Measure->GetBaseline(Font)));
}

// ------------------------------------------------------------------ widget

void SDrawListWidget::Construct(const FArguments& InArgs)
{
	IdealSize = InArgs._DesiredSize;
}

int32 SDrawListWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const TSharedPtr<const ss::ui::DrawList> Snapshot = List;
	if (!Snapshot.IsValid() || Snapshot->Width <= 0.0f || Snapshot->Height <= 0.0f)
	{
		return LayerId;
	}
	const ss::ui::DrawList& L = *Snapshot;
	const FVector2D Local = AllottedGeometry.GetLocalSize();
	const float Sx = static_cast<float>(Local.X) / L.Width;
	const float Sy = static_cast<float>(Local.Y) / L.Height;
	const FSlateRenderTransform& Xf = AllottedGeometry.GetAccumulatedRenderTransform();
	int32 Layer = LayerId;
	TArray<FSlateVertex> Verts;
	TArray<SlateIndex> Indices;
	for (const ss::ui::DrawCmd& Cmd : L.Cmds)
	{
		switch (Cmd.Type)
		{
		case ss::ui::DrawCmd::Kind::Triangles:
		{
			Verts.Reset(static_cast<int32>(Cmd.VertexCount));
			Indices.Reset(static_cast<int32>(Cmd.IndexCount));
			for (uint32 I = 0; I < Cmd.VertexCount; ++I)
			{
				const ss::ui::Vertex& V = L.Vertices[Cmd.VertexStart + I];
				const FColor Col(
					static_cast<uint8>(FMath::Clamp(V.Col.R * 255.0f + 0.5f, 0.0f, 255.0f)),
					static_cast<uint8>(FMath::Clamp(V.Col.G * 255.0f + 0.5f, 0.0f, 255.0f)),
					static_cast<uint8>(FMath::Clamp(V.Col.B * 255.0f + 0.5f, 0.0f, 255.0f)),
					static_cast<uint8>(FMath::Clamp(V.Col.A * 255.0f + 0.5f, 0.0f, 255.0f)));
				Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Xf, FVector2f(V.X * Sx, V.Y * Sy), FVector2f(0.0f, 0.0f), FVector2f(1.0f, 1.0f), Col));
			}
			for (uint32 I = 0; I < Cmd.IndexCount; ++I)
			{
				Indices.Add(static_cast<SlateIndex>(L.Indices[Cmd.IndexStart + I]));
			}
			FSlateDrawElement::MakeCustomVerts(OutDrawElements, ++Layer, FSlateResourceHandle(), Verts, Indices, nullptr, 0, 0);
			break;
		}
		case ss::ui::DrawCmd::Kind::Text:
		{
			const ss::ui::TextItem& T = Cmd.Text;
			const FSlateFontInfo Font = FSlateTextMeasurer::FontFor(T.Face, T.SizePx * Sy);
			const FString Str = FString(UTF8_TO_TCHAR(T.Text.c_str()));
			const FVector2D Offset(static_cast<double>(T.X * Sx), static_cast<double>(T.Y * Sy));
			const FVector2D Box(4096.0, static_cast<double>(T.SizePx * Sy) * 2.0);
			const FPaintGeometry TextGeometry = AllottedGeometry.ToPaintGeometry(Box, FSlateLayoutTransform(1.0f, Offset));
			FSlateDrawElement::MakeText(OutDrawElements, ++Layer, TextGeometry, Str, Font, ESlateDrawEffect::None, FLinearColor(FColor(
				static_cast<uint8>(FMath::Clamp(T.Col.R * 255.0f + 0.5f, 0.0f, 255.0f)),
				static_cast<uint8>(FMath::Clamp(T.Col.G * 255.0f + 0.5f, 0.0f, 255.0f)),
				static_cast<uint8>(FMath::Clamp(T.Col.B * 255.0f + 0.5f, 0.0f, 255.0f)),
				static_cast<uint8>(FMath::Clamp(T.Col.A * 255.0f + 0.5f, 0.0f, 255.0f)))));
			break;
		}
		case ss::ui::DrawCmd::Kind::PushClip:
		{
			const FVector2D A = AllottedGeometry.LocalToAbsolute(FVector2D(Cmd.Clip.X * Sx, Cmd.Clip.Y * Sy));
			const FVector2D B = AllottedGeometry.LocalToAbsolute(FVector2D((Cmd.Clip.X + Cmd.Clip.W) * Sx, (Cmd.Clip.Y + Cmd.Clip.H) * Sy));
			OutDrawElements.PushClip(FSlateClippingZone(FSlateRect(static_cast<float>(A.X), static_cast<float>(A.Y), static_cast<float>(B.X), static_cast<float>(B.Y))));
			break;
		}
		case ss::ui::DrawCmd::Kind::PopClip:
			OutDrawElements.PopClip();
			break;
		}
	}
	return Layer;
}
