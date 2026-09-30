#include "SlateDrawList.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

// ------------------------------------------------------------------ text measurement

bool FSlateTextMeasurer::IsAvailable()
{
	return FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer() != nullptr;
}

FSlateFontInfo FSlateTextMeasurer::FontFor(ss::ui::Font Face, float SizePx)
{
	const TCHAR* Typeface = Face == ss::ui::Font::Bold ? TEXT("Bold") : Face == ss::ui::Font::Mono ? TEXT("Mono") : TEXT("Regular");
	FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(FName(Typeface), 10);
	Font.Size = SizePx * 0.75f;
	return Font;
}

float FSlateTextMeasurer::Width(const std::string& Text, ss::ui::Font Face, float SizePx) const
{
	if (Text.empty() || !IsAvailable())
	{
		return static_cast<float>(Text.size()) * SizePx * 0.55f;
	}
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FString Str = FString(UTF8_TO_TCHAR(Text.c_str()));
	return static_cast<float>(Measure->Measure(Str, FontFor(Face, SizePx)).X);
}

float FSlateTextMeasurer::Ascent(ss::ui::Font Face, float SizePx) const
{
	if (!IsAvailable())
	{
		return SizePx * 0.93f;
	}
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo Font = FontFor(Face, SizePx);
	// The baseline is reported as a negative offset from the bottom of the line.
	return static_cast<float>(Measure->GetMaxCharacterHeight(Font)) + static_cast<float>(Measure->GetBaseline(Font));
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
