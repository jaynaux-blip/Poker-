#include "ShortStack/UI/StreamArt.h"

#include <algorithm>
#include <cmath>

namespace ss
{
namespace ui
{
namespace streamart
{
namespace streamart_detail
{
template <typename T>
float Fl(T V)
{
	return static_cast<float>(V);
}

Color Alpha(const Color& Cl, float A)
{
	return {Cl.R, Cl.G, Cl.B, Cl.A * A};
}

/** Unit coordinates in the largest square centered in R. */
struct Box
{
	float X0 = 0.0f;
	float Y0 = 0.0f;
	float S = 1.0f;
	explicit Box(const Rect& R)
	{
		S = std::min(R.W, R.H);
		X0 = R.X + (R.W - S) * 0.5f;
		Y0 = R.Y + (R.H - S) * 0.5f;
	}
	Vec2 P(float U, float V) const { return {X0 + U * S, Y0 + V * S}; }
	float X(float U) const { return X0 + U * S; }
	float Y(float V) const { return Y0 + V * S; }
	float L(float U) const { return U * S; }
	Rect Rc(float U, float V, float W, float H) const { return {X0 + U * S, Y0 + V * S, W * S, H * S}; }
};

std::vector<Vec2> Pts(const Box& B, std::initializer_list<std::pair<float, float>> L)
{
	std::vector<Vec2> Out;
	for (const auto& P : L)
	{
		Out.push_back(B.P(P.first, P.second));
	}
	return Out;
}

/** Darkens toward the edges of R (gradients are per vertex, so four edge bands rather than one radial fill). */
void Vignette(Canvas& C, const Rect& R, const Color& Edge, float Depth)
{
	const Color None{Edge.R, Edge.G, Edge.B, 0.0f};
	const float D = std::min(R.W, R.H) * Depth;
	C.FillRect({R.X, R.Y, R.W, D}, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + D}, Edge, None));
	C.FillRect({R.X, R.Y + R.H - D, R.W, D}, Paint::Linear({0.0f, R.Y + R.H}, {0.0f, R.Y + R.H - D}, Edge, None));
	C.FillRect({R.X, R.Y, D, R.H}, Paint::Linear({R.X, 0.0f}, {R.X + D, 0.0f}, Edge, None));
	C.FillRect({R.X + R.W - D, R.Y, D, R.H}, Paint::Linear({R.X + R.W, 0.0f}, {R.X + R.W - D, 0.0f}, Edge, None));
}

void Shadow(Canvas& C, const Box& B, float U, float V, float W)
{
	C.FillEllipse(B.X(U), B.Y(V), B.L(W), B.L(W * 0.12f), Rgba(0, 0, 0, 0.18f));
}

/** A screen showing a slice of a RiverLine table. */
void TinyTable(Canvas& C, const Rect& R)
{
	C.FillRect(R, Paint::Linear({R.X, R.Y}, {R.X, R.Y + R.H}, Hex(0x0e1726), Hex(0x0a111c)));
	C.FillEllipse(R.X + R.W * 0.5f, R.Y + R.H * 0.56f, R.W * 0.36f, R.H * 0.28f, Paint::Radial({R.X + R.W * 0.5f, R.Y + R.H * 0.5f}, 0.0f, {R.X + R.W * 0.5f, R.Y + R.H * 0.5f}, R.W * 0.4f, Hex(0x15885f), 0.6f, Hex(0x0f6a4e), Hex(0x083a2b)));
	C.StrokeEllipse(R.X + R.W * 0.5f, R.Y + R.H * 0.56f, R.W * 0.36f, R.H * 0.28f, Hex(0x3a2a1a), std::max(1.0f, R.H * 0.04f));
	for (int K = 0; K < 3; ++K)
	{
		C.FillRoundRect({R.X + R.W * (0.4f + 0.07f * Fl(K)), R.Y + R.H * 0.5f, R.W * 0.055f, R.H * 0.14f}, 1.0f, Hex(0xf5f5f0));
	}
	C.FillRect({R.X, R.Y, R.W, std::max(1.0f, R.H * 0.08f)}, Hex(0x111c2e));
}

void Monitor(Canvas& C, const Box& B, float U, float V, float W, float H, uint32_t Hue, bool Wide)
{
	const float Bezel = Wide ? 0.012f : 0.022f;
	// Stand.
	C.FillPolygon(Pts(B, {{U + W * 0.44f, V + H}, {U + W * 0.56f, V + H}, {U + W * 0.58f, V + H + 0.09f}, {U + W * 0.42f, V + H + 0.09f}}), Hex(0x2a3142));
	C.FillRoundRect(B.Rc(U + W * 0.3f, V + H + 0.085f, W * 0.4f, 0.025f), B.L(0.01f), Hex(0x1f2533));
	C.FillRoundRect(B.Rc(U, V, W, H), B.L(0.015f), Hex(0x151a24));
	TinyTable(C, B.Rc(U + Bezel, V + Bezel, W - Bezel * 2.0f, H - Bezel * 2.5f));
	C.FillRect(B.Rc(U + W * 0.47f, V + H - Bezel * 0.8f, W * 0.06f, Bezel * 0.35f), Hex(Hue));
}

void Ram(Canvas& C, const Box& B, uint32_t Hue)
{
	for (int K = 0; K < 2; ++K)
	{
		C.Save();
		C.Translate(B.X(0.5f), B.Y(0.5f));
		C.Rotate(-0.35f);
		C.Translate(B.L(-0.06f + 0.14f * Fl(K)), B.L(0.05f * Fl(K)));
		const Rect Pcb{B.L(-0.36f), B.L(-0.1f), B.L(0.72f), B.L(0.2f)};
		C.FillRoundRect(Pcb, B.L(0.015f), Hex(0x14532d));
		C.FillRoundRect({Pcb.X, Pcb.Y, Pcb.W, Pcb.H * 0.72f}, B.L(0.015f), Paint::Linear({0.0f, Pcb.Y}, {0.0f, Pcb.Y + Pcb.H}, Hex(0x1e293b), Hex(0x0f172a)));
		C.FillRect({Pcb.X, Pcb.Y + Pcb.H * 0.28f, Pcb.W, Pcb.H * 0.06f}, Hex(Hue));
		for (int G = 0; G < 18; ++G)
		{
			C.FillRect({Pcb.X + B.L(0.02f) + Fl(G) * B.L(0.038f), Pcb.Y + Pcb.H * 0.82f, B.L(0.024f), Pcb.H * 0.16f}, Hex(0xe0b04a));
		}
		C.Restore();
	}
}

void Tower(Canvas& C, const Box& B, float U, float W, uint32_t Hue, double Time)
{
	const Rect Body = B.Rc(U, 0.16f, W, 0.68f);
	C.FillRoundRect(Body, B.L(0.025f), Paint::Linear({Body.X, Body.Y}, {Body.X + Body.W, Body.Y + Body.H}, Hex(0x2b3245), Hex(0x111622)));
	const Rect Glass{Body.X + B.L(0.03f), Body.Y + B.L(0.04f), Body.W - B.L(0.06f), Body.H - B.L(0.1f)};
	C.FillRoundRect(Glass, B.L(0.012f), Hex(0x0b0f17));
	for (int K = 0; K < 3; ++K)
	{
		const float Cy = Glass.Y + Glass.H * (0.2f + 0.3f * Fl(K));
		const float Cx = Glass.X + Glass.W * 0.5f;
		const float Rr = std::min(Glass.W, Glass.H * 0.3f) * 0.36f;
		C.FillCircle(Cx, Cy, Rr * 1.25f, Alpha(Hex(Hue), 0.18f));
		C.StrokeEllipse(Cx, Cy, Rr, Rr, Hex(Hue), std::max(1.0f, Rr * 0.18f));
		const float Spin = Fl(Time * 3.0 + K);
		for (int Bl = 0; Bl < 4; ++Bl)
		{
			const float A = Spin + Fl(Bl) * Pi * 0.5f;
			C.StrokePolyline({{Cx, Cy}, {Cx + std::cos(A) * Rr * 0.8f, Cy + std::sin(A) * Rr * 0.8f}}, false, Alpha(Hex(0xffffff), 0.35f), std::max(1.0f, Rr * 0.12f));
		}
	}
	C.FillRect({Body.X + B.L(0.02f), Body.Y + Body.H - B.L(0.04f), Body.W - B.L(0.04f), B.L(0.012f)}, Hex(Hue));
}

void Webcam(Canvas& C, const Box& B, bool Pro, uint32_t Hue)
{
	const float W = Pro ? 0.62f : 0.46f;
	// Clip.
	C.FillPolygon(Pts(B, {{0.42f, 0.62f}, {0.58f, 0.62f}, {0.62f, 0.78f}, {0.38f, 0.78f}}), Hex(0x1f2533));
	C.FillRoundRect(B.Rc(0.3f, 0.76f, 0.4f, 0.05f), B.L(0.02f), Hex(0x1f2533));
	const Rect Body = B.Rc(0.5f - W * 0.5f, 0.36f, W, 0.28f);
	C.FillRoundRect(Body, B.L(0.13f), Paint::Linear({0.0f, Body.Y}, {0.0f, Body.Y + Body.H}, Hex(0x2f3646), Hex(0x10141c)));
	C.FillCircle(B.X(0.5f), B.Y(0.5f), B.L(0.11f), Hex(0x05070b));
	C.FillCircle(B.X(0.5f), B.Y(0.5f), B.L(0.075f), Paint::Radial(B.P(0.48f, 0.48f), 0.0f, B.P(0.5f, 0.5f), B.L(0.08f), Hex(0x3b82f6), 0.5f, Hex(0x1e3a8a), Hex(0x020617)));
	C.FillCircle(B.X(0.475f), B.Y(0.475f), B.L(0.02f), Rgba(255, 255, 255, 0.7f));
	if (Pro)
	{
		C.FillCircle(B.X(0.28f), B.Y(0.5f), B.L(0.03f), Hex(0x111827));
		C.FillCircle(B.X(0.72f), B.Y(0.5f), B.L(0.03f), Hex(0x111827));
	}
	C.FillCircle(B.X(0.5f + W * 0.32f), B.Y(0.42f), B.L(0.012f), Hex(Hue));
}

void Mirrorless(Canvas& C, const Box& B)
{
	const Rect Body = B.Rc(0.18f, 0.32f, 0.64f, 0.4f);
	C.FillRoundRect(Body, B.L(0.05f), Paint::Linear({0.0f, Body.Y}, {0.0f, Body.Y + Body.H}, Hex(0x2b2f38), Hex(0x0d0f14)));
	C.FillRoundRect(B.Rc(0.22f, 0.26f, 0.2f, 0.08f), B.L(0.02f), Hex(0x1c1f27));
	C.FillRoundRect(B.Rc(0.62f, 0.28f, 0.12f, 0.05f), B.L(0.02f), Hex(0x3a3f4b));
	C.FillCircle(B.X(0.52f), B.Y(0.53f), B.L(0.2f), Hex(0x050608));
	C.StrokeEllipse(B.X(0.52f), B.Y(0.53f), B.L(0.19f), B.L(0.19f), Hex(0x3a3f4b), B.L(0.025f));
	C.FillCircle(B.X(0.52f), B.Y(0.53f), B.L(0.12f), Paint::Radial(B.P(0.5f, 0.5f), 0.0f, B.P(0.52f, 0.53f), B.L(0.12f), Hex(0x7c3aed), 0.5f, Hex(0x1e1b4b), Hex(0x000000)));
	C.FillCircle(B.X(0.48f), B.Y(0.48f), B.L(0.03f), Rgba(255, 255, 255, 0.6f));
	C.FillRect(B.Rc(0.2f, 0.4f, 0.04f, 0.24f), Hex(0x3a3f4b));
	C.FillCircle(B.X(0.74f), B.Y(0.38f), B.L(0.014f), Hex(0xef4444));
}

void MicUsb(Canvas& C, const Box& B, uint32_t Hue)
{
	C.FillRoundRect(B.Rc(0.32f, 0.8f, 0.36f, 0.05f), B.L(0.02f), Hex(0x1f2533));
	C.FillRect(B.Rc(0.48f, 0.62f, 0.04f, 0.2f), Hex(0x2a3142));
	C.StrokeArc(B.X(0.5f), B.Y(0.5f), B.L(0.17f), 0.15f, Pi - 0.15f, Hex(0x2a3142), B.L(0.03f));
	const Rect Cap = B.Rc(0.38f, 0.14f, 0.24f, 0.48f);
	C.FillRoundRect(Cap, B.L(0.12f), Paint::Linear({Cap.X, 0.0f}, {Cap.X + Cap.W, 0.0f}, Hex(0x4b5563), Hex(0x1f2937)));
	for (int K = 0; K < 7; ++K)
	{
		C.FillRect({Cap.X + B.L(0.03f), Cap.Y + B.L(0.04f) + Fl(K) * B.L(0.032f), Cap.W - B.L(0.06f), B.L(0.01f)}, Rgba(0, 0, 0, 0.35f));
	}
	C.FillCircle(B.X(0.5f), B.Y(0.5f), B.L(0.018f), Hex(Hue));
}

void MicXlr(Canvas& C, const Box& B, uint32_t Hue)
{
	// Boom arm.
	C.StrokePolyline(Pts(B, {{0.86f, 0.86f}, {0.78f, 0.5f}, {0.56f, 0.3f}}), false, Hex(0x1f2533), B.L(0.035f), true);
	C.StrokePolyline(Pts(B, {{0.82f, 0.86f}, {0.74f, 0.5f}}), false, Hex(0x374151), B.L(0.012f), true);
	C.FillRoundRect(B.Rc(0.76f, 0.84f, 0.16f, 0.05f), B.L(0.02f), Hex(0x111827));
	C.Save();
	C.Translate(B.X(0.4f), B.Y(0.36f));
	C.Rotate(-0.5f);
	const Rect Body{B.L(-0.1f), B.L(-0.24f), B.L(0.2f), B.L(0.48f)};
	C.FillRoundRect(Body, B.L(0.08f), Paint::Linear({Body.X, 0.0f}, {Body.X + Body.W, 0.0f}, Hex(0x374151), Hex(0x0b0f17)));
	C.FillRoundRect({Body.X, Body.Y, Body.W, B.L(0.2f)}, B.L(0.08f), Hex(0x111827));
	for (int K = 0; K < 5; ++K)
	{
		C.FillRect({Body.X + B.L(0.02f), Body.Y + B.L(0.03f) + Fl(K) * B.L(0.035f), Body.W - B.L(0.04f), B.L(0.008f)}, Rgba(255, 255, 255, 0.12f));
	}
	C.FillRect({Body.X, Body.Y + B.L(0.3f), Body.W, B.L(0.02f)}, Hex(Hue));
	C.Restore();
}

void RingLight(Canvas& C, const Box& B, double Time)
{
	C.FillRect(B.Rc(0.485f, 0.55f, 0.03f, 0.33f), Hex(0x2a3142));
	C.StrokePolyline(Pts(B, {{0.5f, 0.86f}, {0.36f, 0.94f}}), false, Hex(0x2a3142), B.L(0.02f), true);
	C.StrokePolyline(Pts(B, {{0.5f, 0.86f}, {0.64f, 0.94f}}), false, Hex(0x2a3142), B.L(0.02f), true);
	const float Pulse = 0.85f + 0.15f * Fl(std::sin(Time * 2.0));
	C.FillCircle(B.X(0.5f), B.Y(0.35f), B.L(0.32f), Alpha(Hex(0xfff3c4), 0.12f * Pulse));
	C.StrokeEllipse(B.X(0.5f), B.Y(0.35f), B.L(0.24f), B.L(0.24f), Hex(0xfffbeb), B.L(0.07f));
	C.StrokeEllipse(B.X(0.5f), B.Y(0.35f), B.L(0.24f), B.L(0.24f), Alpha(Hex(0xfde68a), 0.6f), B.L(0.02f));
	C.FillRoundRect(B.Rc(0.45f, 0.27f, 0.1f, 0.17f), B.L(0.015f), Hex(0x111827));
}

void KeyLights(Canvas& C, const Box& B)
{
	for (int K = 0; K < 2; ++K)
	{
		const float U = K == 0 ? 0.24f : 0.76f;
		C.FillRect(B.Rc(U - 0.012f, 0.45f, 0.024f, 0.42f), Hex(0x2a3142));
		C.FillRoundRect(B.Rc(U - 0.07f, 0.86f, 0.14f, 0.035f), B.L(0.015f), Hex(0x1f2533));
		C.Save();
		C.Translate(B.X(U), B.Y(0.32f));
		C.Rotate(K == 0 ? 0.25f : -0.25f);
		C.FillCircle(0.0f, 0.0f, B.L(0.22f), Rgba(255, 236, 200, 0.12f));
		C.FillRoundRect({B.L(-0.16f), B.L(-0.12f), B.L(0.32f), B.L(0.24f)}, B.L(0.025f), Hex(0x1f2937));
		C.FillRoundRect({B.L(-0.14f), B.L(-0.1f), B.L(0.28f), B.L(0.2f)}, B.L(0.02f), Paint::Linear({0.0f, B.L(-0.1f)}, {0.0f, B.L(0.1f)}, Hex(0xfffbeb), Hex(0xfde68a)));
		C.Restore();
	}
}

void GreenScreen(Canvas& C, const Box& B)
{
	C.FillRect(B.Rc(0.47f, 0.08f, 0.06f, 0.06f), Hex(0x2a3142));
	C.FillRect(B.Rc(0.2f, 0.12f, 0.6f, 0.68f), Paint::Linear(B.P(0.2f, 0.0f), B.P(0.8f, 0.0f), Hex(0x22c55e), Hex(0x15803d)));
	for (int K = 1; K < 5; ++K)
	{
		C.FillRect(B.Rc(0.2f + 0.12f * Fl(K), 0.12f, 0.004f, 0.68f), Rgba(0, 0, 0, 0.08f));
	}
	C.FillRoundRect(B.Rc(0.16f, 0.78f, 0.68f, 0.1f), B.L(0.02f), Paint::Linear(B.P(0.0f, 0.78f), B.P(0.0f, 0.88f), Hex(0x374151), Hex(0x111827)));
}

void Overlay(Canvas& C, const Box& B, uint32_t Hue)
{
	const Rect Scr = B.Rc(0.08f, 0.18f, 0.84f, 0.6f);
	C.FillRoundRect(Scr, B.L(0.03f), Hex(0x0f0b17));
	TinyTable(C, {Scr.X + B.L(0.02f), Scr.Y + B.L(0.02f), Scr.W * 0.66f, Scr.H - B.L(0.04f)});
	const Rect Cam{Scr.X + Scr.W * 0.7f, Scr.Y + Scr.H * 0.5f, Scr.W * 0.27f, Scr.H * 0.42f};
	C.FillRoundRect(Cam, B.L(0.015f), Hex(0x1e1b4b));
	C.StrokeRoundRect(Cam, B.L(0.015f), Hex(Hue), B.L(0.012f));
	C.FillCircle(Cam.X + Cam.W * 0.5f, Cam.Y + Cam.H * 0.45f, Cam.H * 0.18f, Hex(0xe0b48a));
	C.FillEllipse(Cam.X + Cam.W * 0.5f, Cam.Y + Cam.H, Cam.W * 0.32f, Cam.H * 0.32f, Hex(Hue));
	C.FillRoundRect({Scr.X + Scr.W * 0.7f, Scr.Y + B.L(0.04f), Scr.W * 0.27f, B.L(0.08f)}, B.L(0.02f), Hex(kast::Lime));
	C.FillRoundRect({Scr.X + Scr.W * 0.7f, Scr.Y + B.L(0.16f), Scr.W * 0.27f, B.L(0.03f)}, B.L(0.015f), Rgba(255, 255, 255, 0.15f));
	C.FillRoundRect({Scr.X + Scr.W * 0.7f, Scr.Y + B.L(0.16f), Scr.W * 0.17f, B.L(0.03f)}, B.L(0.015f), Hex(Hue));
}

void Router(Canvas& C, const Box& B, uint32_t Hue, double Time)
{
	for (int K = 0; K < 3; ++K)
	{
		const float U = 0.3f + 0.2f * Fl(K);
		C.StrokePolyline(Pts(B, {{U, 0.56f}, {U + (Fl(K) - 1.0f) * 0.08f, 0.2f}}), false, Hex(0x1f2533), B.L(0.03f), true);
	}
	const Rect Body = B.Rc(0.16f, 0.54f, 0.68f, 0.2f);
	C.FillRoundRect(Body, B.L(0.05f), Paint::Linear({0.0f, Body.Y}, {0.0f, Body.Y + Body.H}, Hex(0xf8fafc), Hex(0xcbd5e1)));
	for (int K = 0; K < 5; ++K)
	{
		const bool On = std::fmod(Time * 3.0 + K * 0.37, 1.0) < 0.6;
		C.FillCircle(B.X(0.28f + 0.1f * Fl(K)), B.Y(0.64f), B.L(0.016f), On ? Hex(Hue) : Hex(0x64748b));
	}
	for (int K = 1; K <= 3; ++K)
	{
		C.StrokeArc(B.X(0.5f), B.Y(0.24f), B.L(0.06f * Fl(K)), Pi * 1.2f, Pi * 1.8f, Alpha(Hex(Hue), 0.9f - 0.2f * Fl(K)), B.L(0.018f), true);
	}
}

void Chair(Canvas& C, const Box& B, uint32_t Hue)
{
	C.FillRect(B.Rc(0.48f, 0.66f, 0.04f, 0.16f), Hex(0x1f2533));
	for (int K = 0; K < 5; ++K)
	{
		const float A = Fl(K) * Pi * 0.4f + 0.3f;
		C.StrokePolyline({B.P(0.5f, 0.84f), {B.X(0.5f) + std::cos(A) * B.L(0.2f), B.Y(0.86f) + std::sin(A) * B.L(0.04f)}}, false, Hex(0x1f2533), B.L(0.025f), true);
		C.FillCircle(B.X(0.5f) + std::cos(A) * B.L(0.2f), B.Y(0.88f) + std::sin(A) * B.L(0.04f), B.L(0.02f), Hex(0x0b0f17));
	}
	C.FillRoundRect(B.Rc(0.26f, 0.58f, 0.48f, 0.1f), B.L(0.04f), Hex(0x1f2937));
	const Rect Back = B.Rc(0.3f, 0.08f, 0.4f, 0.52f);
	C.FillRoundRect(Back, B.L(0.08f), Paint::Linear({Back.X, 0.0f}, {Back.X + Back.W, 0.0f}, Hex(0x374151), Hex(0x111827)));
	C.FillRoundRect({Back.X + B.L(0.05f), Back.Y + B.L(0.06f), Back.W - B.L(0.1f), Back.H - B.L(0.14f)}, B.L(0.05f), Alpha(Hex(Hue), 0.85f));
	for (int K = 0; K < 6; ++K)
	{
		C.FillRect({Back.X + B.L(0.06f), Back.Y + B.L(0.1f) + Fl(K) * B.L(0.055f), Back.W - B.L(0.12f), B.L(0.008f)}, Rgba(0, 0, 0, 0.18f));
	}
	C.FillRoundRect(B.Rc(0.2f, 0.46f, 0.08f, 0.04f), B.L(0.02f), Hex(0x111827));
	C.FillRoundRect(B.Rc(0.72f, 0.46f, 0.08f, 0.04f), B.L(0.02f), Hex(0x111827));
}

void Espresso(Canvas& C, const Box& B, double Time)
{
	const Rect Body = B.Rc(0.22f, 0.16f, 0.56f, 0.66f);
	C.FillRoundRect(Body, B.L(0.05f), Paint::Linear({Body.X, 0.0f}, {Body.X + Body.W, 0.0f}, Hex(0xe5e7eb), Hex(0x9ca3af)));
	C.FillRoundRect(B.Rc(0.26f, 0.2f, 0.48f, 0.12f), B.L(0.03f), Hex(0x1f2937));
	C.FillCircle(B.X(0.36f), B.Y(0.26f), B.L(0.025f), Hex(0x22c55e));
	C.FillRect(B.Rc(0.3f, 0.36f, 0.4f, 0.32f), Hex(0x111827));
	C.FillRoundRect(B.Rc(0.36f, 0.38f, 0.28f, 0.06f), B.L(0.02f), Hex(0x6b7280));
	C.FillRoundRect(B.Rc(0.4f, 0.55f, 0.2f, 0.13f), B.L(0.03f), Hex(0xfafafa));
	C.FillEllipse(B.X(0.5f), B.Y(0.565f), B.L(0.085f), B.L(0.02f), Hex(0x7c4a1e));
	for (int K = 0; K < 2; ++K)
	{
		std::vector<Vec2> Steam;
		for (int I = 0; I <= 8; ++I)
		{
			const float T = Fl(I) / 8.0f;
			Steam.push_back(B.P(0.46f + 0.08f * Fl(K) + 0.02f * std::sin(T * 6.0f + Fl(Time * 2.0)), 0.5f - 0.12f * T));
		}
		C.StrokePolyline(Steam, false, Rgba(255, 255, 255, 0.35f), B.L(0.012f), true);
	}
	C.FillRect(B.Rc(0.22f, 0.76f, 0.56f, 0.06f), Hex(0x4b5563));
}

void Mattress(Canvas& C, const Box& B, uint32_t Hue)
{
	C.FillPolygon(Pts(B, {{0.1f, 0.5f}, {0.7f, 0.5f}, {0.92f, 0.38f}, {0.34f, 0.38f}}), Hex(0xf1f5f9));
	C.FillPolygon(Pts(B, {{0.1f, 0.5f}, {0.7f, 0.5f}, {0.7f, 0.68f}, {0.1f, 0.68f}}), Paint::Linear(B.P(0.0f, 0.5f), B.P(0.0f, 0.68f), Hex(0xe2e8f0), Hex(0xcbd5e1)));
	C.FillPolygon(Pts(B, {{0.7f, 0.5f}, {0.92f, 0.38f}, {0.92f, 0.56f}, {0.7f, 0.68f}}), Hex(0x94a3b8));
	C.FillRect(B.Rc(0.1f, 0.56f, 0.6f, 0.035f), Hex(Hue));
	C.FillPolygon(Pts(B, {{0.7f, 0.56f}, {0.92f, 0.44f}, {0.92f, 0.475f}, {0.7f, 0.595f}}), Alpha(Hex(Hue), 0.7f));
	C.FillEllipse(B.X(0.62f), B.Y(0.42f), B.L(0.12f), B.L(0.035f), Hex(0xffffff));
	C.StrokeEllipse(B.X(0.62f), B.Y(0.42f), B.L(0.12f), B.L(0.035f), Hex(0xcbd5e1), B.L(0.006f));
}

void Curtains(Canvas& C, const Box& B, uint32_t Hue)
{
	C.FillRect(B.Rc(0.2f, 0.16f, 0.6f, 0.62f), Paint::Linear(B.P(0.0f, 0.16f), B.P(0.0f, 0.78f), Hex(0x0b1630), Hex(0x1e3a8a)));
	C.FillCircle(B.X(0.62f), B.Y(0.3f), B.L(0.05f), Hex(0xfef9c3));
	C.FillCircle(B.X(0.645f), B.Y(0.285f), B.L(0.045f), Hex(0x0e1d3d));
	C.FillRect(B.Rc(0.14f, 0.1f, 0.72f, 0.025f), Hex(0x374151));
	for (int Side = 0; Side < 2; ++Side)
	{
		for (int K = 0; K < 4; ++K)
		{
			const float U = Side == 0 ? 0.14f + 0.05f * Fl(K) : 0.66f + 0.05f * Fl(K);
			C.FillRect(B.Rc(U, 0.12f, 0.05f, 0.74f), Mix(Hex(Hue), Hex(0x000000), K % 2 ? 0.45f : 0.3f));
		}
	}
}

void Dumbbell(Canvas& C, const Box& B)
{
	C.Save();
	C.Translate(B.X(0.5f), B.Y(0.52f));
	C.Rotate(-0.4f);
	C.FillRoundRect({B.L(-0.28f), B.L(-0.025f), B.L(0.56f), B.L(0.05f)}, B.L(0.02f), Hex(0x9ca3af));
	for (int Side = -1; Side <= 1; Side += 2)
	{
		for (int K = 0; K < 2; ++K)
		{
			const float X = Fl(Side) * B.L(0.17f + 0.07f * Fl(K));
			const float H = B.L(0.3f - 0.06f * Fl(K));
			C.FillRoundRect({X - B.L(0.03f), -H / 2.0f, B.L(0.06f), H}, B.L(0.015f), K == 0 ? Hex(0x1f2937) : Hex(0x374151));
		}
	}
	C.Restore();
}

void MealKit(Canvas& C, const Box& B, uint32_t Hue)
{
	C.FillPolygon(Pts(B, {{0.18f, 0.42f}, {0.62f, 0.42f}, {0.82f, 0.32f}, {0.38f, 0.32f}}), Hex(0xd6b98c));
	C.FillRect(B.Rc(0.18f, 0.42f, 0.44f, 0.38f), Paint::Linear(B.P(0.0f, 0.42f), B.P(0.0f, 0.8f), Hex(0xc8a675), Hex(0xa98455)));
	C.FillPolygon(Pts(B, {{0.62f, 0.42f}, {0.82f, 0.32f}, {0.82f, 0.7f}, {0.62f, 0.8f}}), Hex(0x8f6d42));
	C.FillRect(B.Rc(0.18f, 0.5f, 0.44f, 0.06f), Hex(Hue));
	// Leaves poking out.
	for (int K = 0; K < 3; ++K)
	{
		C.Save();
		C.Translate(B.X(0.4f + 0.1f * Fl(K)), B.Y(0.36f));
		C.Rotate(-0.6f + 0.6f * Fl(K));
		C.FillEllipse(0.0f, B.L(-0.1f), B.L(0.04f), B.L(0.11f), Hex(K == 1 ? 0x16a34a : 0x22c55e));
		C.Restore();
	}
}

void ModBot(Canvas& C, const Box& B, uint32_t Hue, double Time)
{
	C.FillRect(B.Rc(0.49f, 0.12f, 0.02f, 0.1f), Hex(0x6b7280));
	C.FillCircle(B.X(0.5f), B.Y(0.12f), B.L(0.03f), Hex(Hue));
	const Rect Head = B.Rc(0.24f, 0.22f, 0.52f, 0.4f);
	C.FillRoundRect(Head, B.L(0.1f), Paint::Linear({0.0f, Head.Y}, {0.0f, Head.Y + Head.H}, Hex(0xe5e7eb), Hex(0x9ca3af)));
	C.FillRoundRect(B.Rc(0.3f, 0.3f, 0.4f, 0.2f), B.L(0.07f), Hex(0x111827));
	const bool Blink = std::fmod(Time, 3.0) < 0.12;
	for (int K = 0; K < 2; ++K)
	{
		C.FillEllipse(B.X(0.41f + 0.18f * Fl(K)), B.Y(0.4f), B.L(0.035f), B.L(Blink ? 0.006f : 0.035f), Hex(Hue));
	}
	// Shield.
	C.FillPolygon(Pts(B, {{0.5f, 0.6f}, {0.68f, 0.66f}, {0.66f, 0.8f}, {0.5f, 0.9f}, {0.34f, 0.8f}, {0.32f, 0.66f}}), Hex(Hue));
	C.StrokePolyline(Pts(B, {{0.43f, 0.75f}, {0.48f, 0.8f}, {0.58f, 0.69f}}), false, Hex(0xffffff), B.L(0.03f), true);
}

void Headphones(Canvas& C, const Box& B, uint32_t Hue)
{
	C.StrokeArc(B.X(0.5f), B.Y(0.52f), B.L(0.3f), Pi * 1.05f, Pi * 1.95f, Hex(0x1f2937), B.L(0.06f), true);
	C.StrokeArc(B.X(0.5f), B.Y(0.52f), B.L(0.3f), Pi * 1.2f, Pi * 1.8f, Hex(Hue), B.L(0.015f), true);
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const Rect Cup{B.X(0.5f + 0.3f * Fl(Side)) - B.L(0.08f), B.Y(0.48f), B.L(0.16f), B.L(0.26f)};
		C.FillRoundRect(Cup, B.L(0.07f), Paint::Linear({0.0f, Cup.Y}, {0.0f, Cup.Y + Cup.H}, Hex(0x374151), Hex(0x111827)));
		C.FillRoundRect({Cup.X + B.L(0.03f), Cup.Y + B.L(0.04f), Cup.W - B.L(0.06f), Cup.H - B.L(0.08f)}, B.L(0.04f), Alpha(Hex(Hue), 0.35f));
	}
}

void Plant(Canvas& C, const Box& B, double Time)
{
	const float Sway = Fl(std::sin(Time * 0.8)) * 0.04f;
	for (int K = 0; K < 7; ++K)
	{
		C.Save();
		C.Translate(B.X(0.5f), B.Y(0.6f));
		C.Rotate(-1.1f + 0.36f * Fl(K) + Sway);
		C.FillEllipse(0.0f, B.L(-0.2f), B.L(0.06f), B.L(0.2f), K % 2 ? Hex(0x16a34a) : Hex(0x22c55e));
		C.StrokePolyline({{0.0f, 0.0f}, {0.0f, B.L(-0.36f)}}, false, Rgba(0, 60, 20, 0.4f), B.L(0.008f));
		C.Restore();
	}
	C.FillPolygon(Pts(B, {{0.32f, 0.58f}, {0.68f, 0.58f}, {0.63f, 0.86f}, {0.37f, 0.86f}}), Paint::Linear(B.P(0.32f, 0.0f), B.P(0.68f, 0.0f), Hex(0xd97757), Hex(0x9a4a2e)));
	C.FillRect(B.Rc(0.3f, 0.56f, 0.4f, 0.05f), Hex(0xc2410c));
}

void MacroPad(Canvas& C, const Box& B, uint32_t Hue, double Time)
{
	const Rect Body = B.Rc(0.12f, 0.24f, 0.76f, 0.5f);
	C.FillRoundRect(Body, B.L(0.05f), Paint::Linear({0.0f, Body.Y}, {0.0f, Body.Y + Body.H}, Hex(0x2b3245), Hex(0x111622)));
	static const uint32_t Keys[] = {0x9b5cff, 0x22c55e, 0xef4444, 0xf59e0b, 0x38bdf8};
	for (int Row = 0; Row < 3; ++Row)
	{
		for (int Col = 0; Col < 5; ++Col)
		{
			const Rect K = B.Rc(0.16f + 0.138f * Fl(Col), 0.29f + 0.14f * Fl(Row), 0.11f, 0.11f);
			C.FillRoundRect(K, B.L(0.02f), Hex(0x0b0f17));
			const bool Lit = static_cast<int>(Time * 2.0) % 15 == Row * 5 + Col;
			C.FillRoundRect({K.X + B.L(0.015f), K.Y + B.L(0.015f), K.W - B.L(0.03f), K.H - B.L(0.03f)}, B.L(0.015f), Alpha(Hex(Keys[(Row + Col) % 5]), Lit ? 1.0f : 0.55f));
		}
	}
	(void)Hue;
}
} // namespace streamart_detail

using namespace streamart_detail;

/** A glowing bar: a soft halo, then a bright core (an LED strip, seen lit). */
void LedBar(Canvas& C, const Rect& R, const Color& Col, float Level)
{
	const float L = std::max(0.0f, std::min(1.6f, Level));
	C.GlowRoundRect(R, R.H * 0.5f, Alpha(Col, 0.75f * std::min(1.0f, L)), R.H * 3.0f + 6.0f * L);
	C.FillRoundRect(R, R.H * 0.5f, Mix(Col, Hex(0xffffff), 0.35f + 0.2f * std::min(1.0f, L - 0.5f)));
	C.FillRoundRect({R.X + R.H * 0.5f, R.Y + R.H * 0.3f, R.W - R.H, R.H * 0.4f}, R.H * 0.2f, Alpha(Hex(0xffffff), 0.55f * std::min(1.0f, L)));
}

void LedKit(Canvas& C, const Box& B, double Time)
{
	// The colour on the box cycles through the seven looks.
	const double Pos = std::fmod(std::max(0.0, Time) / 1.6, static_cast<double>(gear::LedPresetCount));
	const int K = static_cast<int>(Pos);
	const double F = std::min(1.0, (Pos - static_cast<double>(K)) * 3.0);
	const Color Col = Hex(gear::MixRgb(gear::LedColor(K, Time), gear::LedColor((K + 1) % gear::LedPresetCount, Time), F * F * (3.0 - 2.0 * F)));
	// A corner of a dark room, washed in the colour from a strip behind the desk.
	const Rect Wall = B.Rc(0.02f, 0.06f, 0.96f, 0.62f);
	C.FillRoundRect(Wall, B.L(0.05f), Paint::Linear(B.P(0.0f, 0.06f), B.P(0.0f, 0.68f), Hex(0x0c0a14), Hex(0x15111f)));
	C.FillRect({Wall.X + B.L(0.02f), B.Y(0.14f), Wall.W - B.L(0.04f), B.L(0.42f)}, Paint::Linear(B.P(0.0f, 0.56f), B.P(0.0f, 0.14f), Alpha(Col, 0.7f), Alpha(Col, 0.0f)));
	LedBar(C, B.Rc(0.1f, 0.13f, 0.8f, 0.018f), Col, 0.8f); // along the ceiling
	LedBar(C, B.Rc(0.08f, 0.545f, 0.84f, 0.026f), Col, 1.2f); // behind the desk
	// The desk, the laptop's silhouette.
	C.FillRoundRect(B.Rc(0.02f, 0.57f, 0.96f, 0.06f), B.L(0.015f), Hex(0x2a1f17));
	C.FillPolygon(Pts(B, {{0.38f, 0.57f}, {0.62f, 0.57f}, {0.6f, 0.41f}, {0.4f, 0.41f}}), Hex(0x1b1d26));
	C.FillRect(B.Rc(0.41f, 0.43f, 0.18f, 0.11f), Alpha(Col, 0.25f));
	// The reel, the strip coming off it, one LED in each colour.
	C.FillCircle(B.X(0.2f), B.Y(0.8f), B.L(0.13f), Paint::Linear(B.P(0.07f, 0.67f), B.P(0.33f, 0.93f), Hex(0xf8fafc), Hex(0xcbd5e1)));
	C.FillCircle(B.X(0.2f), B.Y(0.8f), B.L(0.05f), Hex(0x94a3b8));
	C.StrokeEllipse(B.X(0.2f), B.Y(0.8f), B.L(0.095f), B.L(0.095f), Hex(0x1f2937), B.L(0.02f));
	std::vector<Vec2> Strip;
	for (int I = 0; I <= 24; ++I)
	{
		const float U = Fl(I) / 24.0f;
		Strip.push_back(B.P(0.3f + U * 0.38f, 0.86f - std::sin(U * 3.1416f) * 0.05f + U * 0.04f));
	}
	C.StrokePolyline(Strip, false, Hex(0xe5e7eb), B.L(0.035f), true);
	for (int I = 0; I < gear::LedPresetCount; ++I)
	{
		const Vec2 P = Strip[static_cast<size_t>(2 + I * 3)];
		const Color Dot = Hex(gear::LedColor(I, Time));
		C.FillCircle(P.X, P.Y, B.L(0.022f), Alpha(Dot, 0.35f));
		C.FillCircle(P.X, P.Y, B.L(0.011f), Mix(Dot, Hex(0xffffff), 0.3f));
	}
	// The remote: seven colour keys.
	const Rect Rm = B.Rc(0.74f, 0.66f, 0.15f, 0.28f);
	C.FillRoundRect({Rm.X + B.L(0.01f), Rm.Y + B.L(0.012f), Rm.W, Rm.H}, B.L(0.04f), Rgba(0, 0, 0, 0.18f));
	C.FillRoundRect(Rm, B.L(0.04f), Paint::Linear({Rm.X, Rm.Y}, {Rm.X + Rm.W, Rm.Y + Rm.H}, Hex(0xf8fafc), Hex(0xd1d5db)));
	C.FillCircle(Rm.X + Rm.W * 0.5f, Rm.Y + B.L(0.04f), B.L(0.017f), Hex(0xef4444));
	for (int I = 0; I < gear::LedPresetCount; ++I)
	{
		const float Kx = Rm.X + Rm.W * (I % 2 == 0 ? 0.32f : 0.68f) + (I == 6 ? Rm.W * 0.18f : 0.0f);
		const float Ky = Rm.Y + B.L(0.09f) + Fl(I / 2) * B.L(0.048f);
		C.FillCircle(Kx, Ky, B.L(0.016f), Hex(gear::LedColor(I, Time)));
	}
}

void Product(Canvas& C, gear::Art A, const Rect& R, uint32_t Hue, double Time)
{
	const Box B(R);
	switch (A)
	{
	case gear::Art::Ram: Ram(C, B, Hue); break;
	case gear::Art::Monitor:
		Shadow(C, B, 0.5f, 0.86f, 0.3f);
		Monitor(C, B, 0.14f, 0.2f, 0.72f, 0.48f, Hue, false);
		break;
	case gear::Art::MonitorWide:
		Shadow(C, B, 0.5f, 0.84f, 0.36f);
		Monitor(C, B, 0.06f, 0.24f, 0.88f, 0.44f, Hue, true);
		break;
	case gear::Art::Tower:
		Shadow(C, B, 0.5f, 0.85f, 0.24f);
		Tower(C, B, 0.32f, 0.36f, Hue, Time);
		break;
	case gear::Art::TowerDual:
		Shadow(C, B, 0.5f, 0.85f, 0.4f);
		Tower(C, B, 0.12f, 0.34f, Hue, Time);
		Tower(C, B, 0.54f, 0.34f, 0x22d3ee, Time + 0.5);
		break;
	case gear::Art::MacroPad: MacroPad(C, B, Hue, Time); break;
	case gear::Art::Webcam: Webcam(C, B, false, Hue); break;
	case gear::Art::WebcamPro: Webcam(C, B, true, Hue); break;
	case gear::Art::Mirrorless: Mirrorless(C, B); break;
	case gear::Art::MicUsb: MicUsb(C, B, Hue); break;
	case gear::Art::MicXlr: MicXlr(C, B, Hue); break;
	case gear::Art::RingLight: RingLight(C, B, Time); break;
	case gear::Art::KeyLights: KeyLights(C, B); break;
	case gear::Art::GreenScreen: GreenScreen(C, B); break;
	case gear::Art::Overlay: Overlay(C, B, Hue); break;
	case gear::Art::Router: Router(C, B, Hue, Time); break;
	case gear::Art::Chair: Chair(C, B, Hue); break;
	case gear::Art::Espresso: Espresso(C, B, Time); break;
	case gear::Art::Mattress: Mattress(C, B, Hue); break;
	case gear::Art::Curtains: Curtains(C, B, Hue); break;
	case gear::Art::Gym: Dumbbell(C, B); break;
	case gear::Art::MealKit: MealKit(C, B, Hue); break;
	case gear::Art::ModBot: ModBot(C, B, Hue, Time); break;
	case gear::Art::Headphones: Headphones(C, B, Hue); break;
	case gear::Art::Plant: Plant(C, B, Time); break;
	case gear::Art::LedKit: LedKit(C, B, Time); break;
	default: break;
	}
}

void Desk(Canvas& C, const Rect& R, const gear::Owned& Owned, bool Live, double Time, const gear::Glow& Leds)
{
	auto Has = [&](const char* Id) { return Owned.count(Id) > 0; };
	auto At = [&](float U, float V) { return Vec2{R.X + U * R.W, R.Y + V * R.H}; };
	auto Rc = [&](float U, float V, float W, float H) { return Rect{R.X + U * R.W, R.Y + V * R.H, W * R.W, H * R.H}; };
	const float S = R.H;
	C.PushClip(R);
	// The wall, the window, the rain.
	C.FillRect(R, Paint::Linear({R.X, R.Y}, {R.X, R.Y + R.H}, Hex(0x1c2233), Hex(0x10141f)));
	const Rect Win = Rc(0.06f, 0.08f, 0.2f, 0.4f);
	C.FillRect(Win, Paint::Linear({0.0f, Win.Y}, {0.0f, Win.Y + Win.H}, Hex(0x0b1630), Hex(0x1e2f57)));
	for (int K = 0; K < 10; ++K)
	{
		const float U = std::fmod(Fl(K) * 0.137f + Fl(Time * 0.05), 1.0f);
		const float V = std::fmod(Fl(K) * 0.311f + Fl(Time * 0.9), 1.0f);
		C.StrokePolyline({{Win.X + Win.W * U, Win.Y + Win.H * V}, {Win.X + Win.W * U - 1.0f, Win.Y + Win.H * V + S * 0.03f}}, false, Rgba(180, 200, 255, 0.35f), 1.0f);
	}
	C.StrokeRoundRect(Win, 2.0f, Hex(0x2b3245), 3.0f);
	C.FillRect({Win.X + Win.W * 0.5f - 1.5f, Win.Y, 3.0f, Win.H}, Hex(0x2b3245));
	if (Has("curtains"))
	{
		C.FillRect({Win.X - S * 0.04f, Win.Y - S * 0.03f, S * 0.07f, Win.H + S * 0.12f}, Hex(0x312e81));
		C.FillRect({Win.X + Win.W - S * 0.03f, Win.Y - S * 0.03f, S * 0.07f, Win.H + S * 0.12f}, Hex(0x312e81));
	}
	if (Has("green-screen"))
	{
		C.FillRect(Rc(0.36f, 0.04f, 0.3f, 0.6f), Paint::Linear(At(0.36f, 0.0f), At(0.66f, 0.0f), Hex(0x16a34a), Hex(0x15803d)));
	}
	// ON AIR.
	{
		const Rect Sign = Rc(0.74f, 0.08f, 0.14f, 0.09f);
		C.FillRoundRect(Sign, 3.0f, Live ? Hex(0x7f1d1d) : Hex(0x2a2f3c));
		if (Live)
		{
			C.GlowRoundRect(Sign, 3.0f, Rgba(239, 68, 68, 0.6f), 12.0f);
		}
		TextStyle T;
		T.Size = S * 0.045f;
		T.Weight = 900;
		T.Col = Live ? Hex(0xfecaca) : Hex(0x4b5563);
		T.HAlign = Align::Center;
		T.VAlign = Baseline::Middle;
		C.Text("ON AIR", Sign.X + Sign.W * 0.5f, Sign.Y + Sign.H * 0.52f, T);
	}
	if (Has("fiber"))
	{
		const Rect Shelf = Rc(0.74f, 0.27f, 0.16f, 0.015f);
		C.FillRect(Shelf, Hex(0x3a2a1a));
		C.FillRoundRect({Shelf.X + S * 0.02f, Shelf.Y - S * 0.035f, Shelf.W * 0.6f, S * 0.035f}, 2.0f, Hex(0xe5e7eb));
		for (int K = 0; K < 3; ++K)
		{
			C.FillCircle(Shelf.X + S * 0.04f + Fl(K) * S * 0.025f, Shelf.Y - S * 0.018f, S * 0.006f, std::fmod(Time * 2.0 + K * 0.3, 1.0) < 0.5 ? Hex(0x2dd4bf) : Hex(0x64748b));
		}
	}
	// The LED kit: a strip along the ceiling, one under the sill, one behind the desk washing the wall.
	const bool Lit = Leds.On && Has(gear::LedKitId);
	const Color Led = Hex(Leds.Rgb);
	const float Lv = Fl(Leds.Level);
	if (Lit)
	{
		C.FillRect(R, Alpha(Led, 0.08f * std::min(1.0f, Lv)));
		C.FillRect({R.X, R.Y, R.W, R.H * 0.3f}, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + R.H * 0.3f}, Alpha(Led, 0.4f * std::min(1.2f, Lv)), Alpha(Led, 0.0f)));
		C.FillRect({R.X, R.Y + R.H * 0.24f, R.W, R.H * 0.42f}, Paint::Linear({0.0f, R.Y + R.H * 0.66f}, {0.0f, R.Y + R.H * 0.24f}, Alpha(Led, 0.55f * std::min(1.2f, Lv)), Alpha(Led, 0.0f)));
		LedBar(C, {R.X + R.W * 0.02f, R.Y + S * 0.015f, R.W * 0.96f, S * 0.014f}, Led, Lv);
		LedBar(C, {Win.X - S * 0.02f, Win.Y + Win.H + S * 0.012f, Win.W + S * 0.04f, S * 0.012f}, Led, Lv * 0.8f);
	}
	// Lights behind the desk.
	if (Has("key-lights"))
	{
		for (int K = 0; K < 2; ++K)
		{
			const float U = K == 0 ? 0.12f : 0.86f;
			C.FillRect(Rc(U, 0.2f, 0.008f, 0.5f), Hex(0x2a3142));
			C.FillCircle(R.X + U * R.W, R.Y + 0.22f * R.H, S * 0.2f, Rgba(255, 236, 200, 0.08f));
			C.FillRoundRect({R.X + U * R.W - S * 0.06f, R.Y + 0.16f * R.H, S * 0.12f, S * 0.08f}, 2.0f, Hex(0xfffbeb));
		}
	}
	if (Has("ring-light"))
	{
		C.FillCircle(R.X + 0.5f * R.W, R.Y + 0.3f * R.H, S * 0.22f, Rgba(255, 243, 196, 0.08f));
		C.StrokeEllipse(R.X + 0.5f * R.W, R.Y + 0.3f * R.H, S * 0.16f, S * 0.16f, Hex(0xfffbeb), S * 0.025f);
	}
	// The desk.
	const float Top = R.Y + 0.66f * R.H;
	C.FillRect({R.X, Top, R.W, S * 0.05f}, Paint::Linear({0.0f, Top}, {0.0f, Top + S * 0.05f}, Hex(0x8a6a48), Hex(0x5c4430)));
	C.FillRect({R.X, Top + S * 0.05f, R.W, R.H}, Hex(0x0c0f17));
	if (Lit)
	{
		LedBar(C, {R.X + R.W * 0.04f, Top - S * 0.012f, R.W * 0.92f, S * 0.012f}, Led, Lv);
		C.FillRect({R.X, Top + S * 0.05f, R.W, S * 0.16f}, Paint::Linear({0.0f, Top + S * 0.05f}, {0.0f, Top + S * 0.21f}, Alpha(Led, 0.3f * std::min(1.0f, Lv)), Alpha(Led, 0.0f)));
	}
	C.FillRect({R.X + R.W * 0.08f, Top + S * 0.05f, S * 0.02f, R.H}, Hex(0x1f2533));
	C.FillRect({R.X + R.W * 0.92f - S * 0.02f, Top + S * 0.05f, S * 0.02f, R.H}, Hex(0x1f2533));
	// Monitors either side, the laptop in the middle.
	auto Screen = [&](const Rect& Sc, float Stand) {
		C.FillRect({Sc.X + Sc.W * 0.46f, Sc.Y + Sc.H, Sc.W * 0.08f, Stand}, Hex(0x2a3142));
		C.FillRoundRect({Sc.X + Sc.W * 0.3f, Sc.Y + Sc.H + Stand - 2.0f, Sc.W * 0.4f, 3.0f}, 1.5f, Hex(0x1f2533));
		C.FillRoundRect(Sc, 2.0f, Hex(0x151a24));
		TinyTable(C, {Sc.X + 2.0f, Sc.Y + 2.0f, Sc.W - 4.0f, Sc.H - 4.0f});
		C.FillRect({Sc.X - 4.0f, Sc.Y - 4.0f, Sc.W + 8.0f, Sc.H + 8.0f}, Rgba(39, 211, 195, 0.03f));
	};
	if (Has("monitor-24"))
	{
		const Rect M = Rc(0.14f, 0.34f, 0.22f, 0.22f);
		Screen(M, Top - (M.Y + M.H));
	}
	if (Has("monitor-27"))
	{
		const Rect M = Rc(0.64f, 0.3f, 0.27f, 0.26f);
		Screen(M, Top - (M.Y + M.H));
	}
	// Laptop.
	{
		const Rect Lid = Rc(0.4f, 0.42f, 0.2f, 0.2f);
		C.FillRoundRect(Lid, 2.0f, Hex(0x1f2533));
		TinyTable(C, {Lid.X + 2.0f, Lid.Y + 2.0f, Lid.W - 4.0f, Lid.H - 3.0f});
		C.FillPolygon({{Lid.X - S * 0.02f, Top}, {Lid.X + Lid.W + S * 0.02f, Top}, {Lid.X + Lid.W, Lid.Y + Lid.H}, {Lid.X, Lid.Y + Lid.H}}, Hex(0x9ca3af));
		if (Has("overlay-pack"))
		{
			C.StrokeRoundRect({Lid.X + Lid.W * 0.68f, Lid.Y + Lid.H * 0.55f, Lid.W * 0.28f, Lid.H * 0.38f}, 1.0f, Hex(kast::Violet), 1.5f);
		}
		// Camera on top.
		if (Has("webcam-720") || Has("webcam-1080"))
		{
			C.FillRoundRect({Lid.X + Lid.W * 0.5f - S * 0.03f, Lid.Y - S * 0.035f, S * 0.06f, S * 0.035f}, S * 0.015f, Hex(0x111827));
			C.FillCircle(Lid.X + Lid.W * 0.5f, Lid.Y - S * 0.018f, S * 0.01f, Hex(0x3b82f6));
			if (Live)
			{
				C.FillCircle(Lid.X + Lid.W * 0.5f + S * 0.022f, Lid.Y - S * 0.024f, S * 0.004f, Hex(0xef4444));
			}
		}
	}
	if (Has("mirrorless"))
	{
		const Vec2 P = At(0.33f, 0.5f);
		C.StrokePolyline({{P.X, P.Y + S * 0.04f}, {P.X - S * 0.03f, Top}}, false, Hex(0x2a3142), 1.5f);
		C.StrokePolyline({{P.X, P.Y + S * 0.04f}, {P.X + S * 0.03f, Top}}, false, Hex(0x2a3142), 1.5f);
		C.FillRoundRect({P.X - S * 0.04f, P.Y, S * 0.08f, S * 0.05f}, 2.0f, Hex(0x111827));
		C.FillCircle(P.X + S * 0.01f, P.Y + S * 0.025f, S * 0.018f, Hex(0x312e81));
	}
	if (Has("tower-mid") || Has("tower-dual"))
	{
		const int Count = Has("tower-dual") ? 2 : 1;
		for (int K = 0; K < Count; ++K)
		{
			const Rect T = Rc(0.92f - 0.075f * Fl(K + 1), 0.48f, 0.065f, 0.18f);
			C.FillRoundRect({T.X, Top - T.H, T.W, T.H}, 2.0f, Hex(0x1f2533));
			for (int F = 0; F < 2; ++F)
			{
				const float Cy = Top - T.H + T.H * (0.3f + 0.4f * Fl(F));
				C.StrokeEllipse(T.X + T.W * 0.5f, Cy, T.W * 0.3f, T.W * 0.3f, Hex(K == 0 ? 0x818cf8 : 0x22d3ee), 1.5f);
			}
		}
	}
	if (Has("mic-xlr"))
	{
		C.StrokePolyline({At(0.06f, 0.66f), At(0.12f, 0.36f), At(0.3f, 0.46f)}, false, Hex(0x2a3142), S * 0.012f, true);
		C.FillRoundRect({R.X + 0.28f * R.W, R.Y + 0.44f * R.H, S * 0.05f, S * 0.09f}, S * 0.02f, Hex(0x111827));
	}
	else if (Has("mic-usb"))
	{
		C.FillRoundRect({R.X + 0.34f * R.W, Top - S * 0.11f, S * 0.04f, S * 0.08f}, S * 0.02f, Hex(0x4b5563));
		C.FillRect({R.X + 0.34f * R.W + S * 0.015f, Top - S * 0.03f, S * 0.01f, S * 0.03f}, Hex(0x2a3142));
	}
	if (Has("macro-pad"))
	{
		C.FillRoundRect({R.X + 0.62f * R.W, Top - S * 0.015f, S * 0.08f, S * 0.02f}, 2.0f, Hex(0x2b3245));
		for (int K = 0; K < 4; ++K)
		{
			C.FillRect({R.X + 0.62f * R.W + S * 0.008f + Fl(K) * S * 0.018f, Top - S * 0.012f, S * 0.012f, S * 0.008f}, Hex(K % 2 ? 0x9b5cff : 0x22c55e));
		}
	}
	if (Has("plant"))
	{
		const Box Pb({R.X + 0.01f * R.W, Top - S * 0.2f, S * 0.2f, S * 0.22f});
		Plant(C, Pb, Time);
	}
	if (Has("espresso"))
	{
		const Box Eb({R.X + 0.88f * R.W - S * 0.2f, Top - S * 0.17f, S * 0.17f, S * 0.18f});
		Espresso(C, Eb, Time);
	}
	if (Has("headphones"))
	{
		C.StrokeArc(R.X + 0.4f * R.W, Top - S * 0.01f, S * 0.04f, Pi, Pi * 2.0f, Hex(0x374151), S * 0.012f);
	}
	if (Has("gym"))
	{
		C.FillRoundRect({R.X + 0.2f * R.W, R.Y + R.H - S * 0.06f, S * 0.12f, S * 0.015f}, 2.0f, Hex(0x6b7280));
		C.FillRoundRect({R.X + 0.2f * R.W - S * 0.01f, R.Y + R.H - S * 0.085f, S * 0.025f, S * 0.06f}, 2.0f, Hex(0x1f2937));
		C.FillRoundRect({R.X + 0.2f * R.W + S * 0.105f, R.Y + R.H - S * 0.085f, S * 0.025f, S * 0.06f}, 2.0f, Hex(0x1f2937));
	}
	if (Has("meal-kit"))
	{
		C.FillRect({R.X + 0.78f * R.W, R.Y + R.H - S * 0.12f, S * 0.12f, S * 0.1f}, Hex(0xb08a5a));
		C.FillRect({R.X + 0.78f * R.W, R.Y + R.H - S * 0.09f, S * 0.12f, S * 0.015f}, Hex(0x4ade80));
	}
	// The chair, from behind.
	{
		const bool Good = Has("chair");
		const Rect Back = Rc(0.42f, 0.74f, 0.16f, 0.4f);
		C.FillRoundRect(Back, S * (Good ? 0.04f : 0.01f), Good ? Paint::Linear({Back.X, 0.0f}, {Back.X + Back.W, 0.0f}, Hex(0x374151), Hex(0x111827)) : Paint(Hex(0x6b4f35)));
		if (Good)
		{
			C.FillRoundRect({Back.X + S * 0.02f, Back.Y + S * 0.03f, Back.W - S * 0.04f, Back.H}, S * 0.03f, Hex(0xfb923c));
		}
		else
		{
			C.FillRect({Back.X + S * 0.015f, Back.Y + S * 0.04f, Back.W - S * 0.03f, S * 0.012f}, Hex(0x4a3524));
			C.FillRect({Back.X + S * 0.015f, Back.Y + S * 0.09f, Back.W - S * 0.03f, S * 0.012f}, Hex(0x4a3524));
		}
	}
	C.PopClip();
}

void Emote(Canvas& C, int E, float X, float Y, float Sz)
{
	const float Cx = X + Sz * 0.5f;
	const float Cy = Y + Sz * 0.5f;
	const float R = Sz * 0.46f;
	auto Face = [&](uint32_t Col) {
		C.FillCircle(Cx, Cy, R, Paint::Radial({Cx - R * 0.3f, Cy - R * 0.3f}, 0.0f, {Cx, Cy}, R, Mix(Hex(Col), Hex(0xffffff), 0.35f), 0.5f, Hex(Col), Mix(Hex(Col), Hex(0x000000), 0.25f)));
	};
	const Color EmoteInk = Hex(0x1b1036);
	switch (static_cast<kast::Emote>(E))
	{
	case kast::Emote::Pog:
		Face(0xfacc15);
		C.FillEllipse(Cx - R * 0.35f, Cy - R * 0.2f, R * 0.17f, R * 0.22f, Hex(0xffffff));
		C.FillEllipse(Cx + R * 0.35f, Cy - R * 0.2f, R * 0.17f, R * 0.22f, Hex(0xffffff));
		C.FillCircle(Cx - R * 0.35f, Cy - R * 0.18f, R * 0.08f, EmoteInk);
		C.FillCircle(Cx + R * 0.35f, Cy - R * 0.18f, R * 0.08f, EmoteInk);
		C.FillEllipse(Cx, Cy + R * 0.4f, R * 0.2f, R * 0.26f, EmoteInk);
		break;
	case kast::Emote::Lul:
		Face(0xfbbf24);
		C.StrokeArc(Cx - R * 0.35f, Cy - R * 0.15f, R * 0.15f, Pi * 1.1f, Pi * 1.9f, EmoteInk, R * 0.1f, true);
		C.StrokeArc(Cx + R * 0.35f, Cy - R * 0.15f, R * 0.15f, Pi * 1.1f, Pi * 1.9f, EmoteInk, R * 0.1f, true);
		C.FillPolygon({{Cx - R * 0.5f, Cy + R * 0.1f}, {Cx + R * 0.5f, Cy + R * 0.1f}, {Cx + R * 0.2f, Cy + R * 0.6f}, {Cx - R * 0.2f, Cy + R * 0.6f}}, EmoteInk);
		C.FillEllipse(Cx - R * 0.7f, Cy + R * 0.05f, R * 0.12f, R * 0.2f, Hex(0x60a5fa));
		C.FillEllipse(Cx + R * 0.7f, Cy + R * 0.05f, R * 0.12f, R * 0.2f, Hex(0x60a5fa));
		break;
	case kast::Emote::Gg:
	{
		C.FillRoundRect({X + Sz * 0.04f, Y + Sz * 0.18f, Sz * 0.92f, Sz * 0.64f}, Sz * 0.16f, Paint::Linear({X, Y}, {X + Sz, Y + Sz}, Hex(0x22c55e), Hex(0x15803d)));
		TextStyle T;
		T.Size = Sz * 0.5f;
		T.Weight = 900;
		T.Col = Hex(0xffffff);
		T.HAlign = Align::Center;
		T.VAlign = Baseline::Middle;
		C.Text("GG", Cx, Cy + Sz * 0.02f, T);
		break;
	}
	case kast::Emote::Rip:
		C.FillRoundRect({Cx - R * 0.6f, Cy - R * 0.8f, R * 1.2f, R * 1.7f}, R * 0.55f, Paint::Linear({0.0f, Cy - R}, {0.0f, Cy + R}, Hex(0xd1d5db), Hex(0x6b7280)));
		C.FillRect({Cx - R * 0.08f, Cy - R * 0.5f, R * 0.16f, R * 0.7f}, Hex(0x374151));
		C.FillRect({Cx - R * 0.3f, Cy - R * 0.3f, R * 0.6f, R * 0.14f}, Hex(0x374151));
		C.FillRect({Cx - R * 0.9f, Cy + R * 0.82f, R * 1.8f, R * 0.14f}, Hex(0x22c55e));
		break;
	case kast::Emote::Hype:
	{
		std::vector<Vec2> P;
		for (int I = 0; I <= 20; ++I)
		{
			const float A = Fl(I) / 20.0f * Pi * 2.0f;
			const float Rr = R * (1.0f - 0.45f * (0.5f - 0.5f * std::cos(A)));
			P.push_back({Cx + std::sin(A) * Rr * 0.8f, Cy + R * 0.2f - std::cos(A) * R * (std::cos(A) > 0.0f ? 1.05f : 0.7f)});
		}
		C.FillPolygon(P, Paint::Linear({0.0f, Cy - R}, {0.0f, Cy + R}, Hex(0xfde047), Hex(0xf97316)));
		C.FillEllipse(Cx, Cy + R * 0.35f, R * 0.3f, R * 0.4f, Hex(0xfef08a));
		break;
	}
	case kast::Emote::Love:
	{
		std::vector<Vec2> P;
		for (int I = 0; I <= 24; ++I)
		{
			const float T = Fl(I) / 24.0f * Pi * 2.0f;
			const float Hx = 16.0f * std::pow(std::sin(T), 3.0f);
			const float Hy = 13.0f * std::cos(T) - 5.0f * std::cos(2.0f * T) - 2.0f * std::cos(3.0f * T) - std::cos(4.0f * T);
			P.push_back({Cx + Hx / 17.0f * R, Cy - Hy / 17.0f * R + R * 0.08f});
		}
		C.FillPolygon(P, Paint::Linear({0.0f, Cy - R}, {0.0f, Cy + R}, Hex(0xff6b9a), Hex(0xe11d48)));
		C.FillEllipse(Cx - R * 0.4f, Cy - R * 0.3f, R * 0.14f, R * 0.1f, Rgba(255, 255, 255, 0.6f));
		break;
	}
	case kast::Emote::Fish:
		C.FillPolygon({{Cx + R * 0.4f, Cy}, {Cx + R * 0.95f, Cy - R * 0.45f}, {Cx + R * 0.95f, Cy + R * 0.45f}}, Hex(0x0284c7));
		C.FillEllipse(Cx - R * 0.1f, Cy, R * 0.7f, R * 0.45f, Paint::Linear({0.0f, Cy - R}, {0.0f, Cy + R}, Hex(0x7dd3fc), Hex(0x0284c7)));
		C.FillCircle(Cx - R * 0.45f, Cy - R * 0.1f, R * 0.1f, EmoteInk);
		break;
	case kast::Emote::Salt:
		C.FillRoundRect({Cx - R * 0.45f, Cy - R * 0.4f, R * 0.9f, R * 1.3f}, R * 0.25f, Paint::Linear({Cx - R, 0.0f}, {Cx + R, 0.0f}, Hex(0xf8fafc), Hex(0xcbd5e1)));
		C.FillRoundRect({Cx - R * 0.45f, Cy - R * 0.85f, R * 0.9f, R * 0.5f}, R * 0.25f, Hex(0x94a3b8));
		for (int K = 0; K < 3; ++K)
		{
			C.FillCircle(Cx - R * 0.2f + Fl(K) * R * 0.2f, Cy - R * 0.6f, R * 0.05f, Hex(0x334155));
		}
		C.FillCircle(Cx - R * 0.75f, Cy - R * 0.1f, R * 0.06f, Hex(0xffffff));
		C.FillCircle(Cx + R * 0.75f, Cy + R * 0.2f, R * 0.05f, Hex(0xffffff));
		break;
	case kast::Emote::Clap:
		for (int K = 0; K < 2; ++K)
		{
			C.Save();
			C.Translate(Cx + (K == 0 ? -R * 0.3f : R * 0.3f), Cy + R * 0.25f);
			C.Rotate(K == 0 ? 0.45f : -0.45f);
			C.FillRoundRect({-R * 0.28f, -R * 0.8f, R * 0.56f, R * 1.25f}, R * 0.26f, K == 0 ? Hex(0xf5c08a) : Hex(0xe0a46c));
			C.FillRoundRect({K == 0 ? R * 0.1f : -R * 0.42f, -R * 0.2f, R * 0.32f, R * 0.18f}, R * 0.09f, K == 0 ? Hex(0xf5c08a) : Hex(0xe0a46c)); // thumb
			C.Restore();
		}
		for (int K = 0; K < 3; ++K)
		{
			const float A = -Pi * 0.5f + (Fl(K) - 1.0f) * 0.6f;
			C.StrokePolyline({{Cx + std::cos(A) * R * 0.78f, Cy - R * 0.25f + std::sin(A) * R * 0.78f}, {Cx + std::cos(A) * R * 1.0f, Cy - R * 0.25f + std::sin(A) * R * 1.0f}}, false, Hex(0xfacc15), R * 0.11f, true);
		}
		break;
	case kast::Emote::Chip:
		C.FillCircle(Cx, Cy, R, Hex(0xdc2626));
		for (int K = 0; K < 6; ++K)
		{
			const float A = Fl(K) * Pi / 3.0f;
			C.FillRoundRect({Cx + std::cos(A) * R * 0.78f - R * 0.12f, Cy + std::sin(A) * R * 0.78f - R * 0.12f, R * 0.24f, R * 0.24f}, R * 0.05f, Hex(0xffffff));
		}
		C.FillCircle(Cx, Cy, R * 0.55f, Hex(0xb91c1c));
		C.StrokeEllipse(Cx, Cy, R * 0.55f, R * 0.55f, Hex(0xffffff), R * 0.08f, R * 0.25f);
		break;
	case kast::Emote::Ship:
		C.FillPolygon({{Cx - R * 0.9f, Cy + R * 0.3f}, {Cx + R * 0.9f, Cy + R * 0.3f}, {Cx + R * 0.6f, Cy + R * 0.75f}, {Cx - R * 0.6f, Cy + R * 0.75f}}, Hex(kast::Violet));
		C.FillRect({Cx - R * 0.04f, Cy - R * 0.9f, R * 0.08f, R * 1.2f}, Hex(0x4c1d95));
		C.FillPolygon({{Cx + R * 0.1f, Cy - R * 0.85f}, {Cx + R * 0.75f, Cy + R * 0.2f}, {Cx + R * 0.1f, Cy + R * 0.2f}}, Hex(kast::Lime));
		C.FillPolygon({{Cx - R * 0.1f, Cy - R * 0.6f}, {Cx - R * 0.65f, Cy + R * 0.2f}, {Cx - R * 0.1f, Cy + R * 0.2f}}, Hex(0xffffff));
		break;
	case kast::Emote::Tilt:
		Face(0xef4444);
		C.StrokePolyline({{Cx - R * 0.6f, Cy - R * 0.45f}, {Cx - R * 0.15f, Cy - R * 0.2f}}, false, EmoteInk, R * 0.12f, true);
		C.StrokePolyline({{Cx + R * 0.6f, Cy - R * 0.45f}, {Cx + R * 0.15f, Cy - R * 0.2f}}, false, EmoteInk, R * 0.12f, true);
		C.StrokeArc(Cx, Cy + R * 0.6f, R * 0.35f, Pi * 1.15f, Pi * 1.85f, EmoteInk, R * 0.12f, true);
		C.FillCircle(Cx - R * 0.8f, Cy - R * 0.9f, R * 0.12f, Rgba(255, 255, 255, 0.7f));
		C.FillCircle(Cx + R * 0.85f, Cy - R * 0.8f, R * 0.1f, Rgba(255, 255, 255, 0.6f));
		break;
	case kast::Emote::Rent:
		C.FillPolygon({{Cx - R * 0.95f, Cy - R * 0.05f}, {Cx, Cy - R * 0.9f}, {Cx + R * 0.95f, Cy - R * 0.05f}}, Hex(0xef4444));
		C.FillRect({Cx - R * 0.7f, Cy - R * 0.1f, R * 1.4f, R * 0.95f}, Hex(0xfef3c7));
		C.FillRect({Cx - R * 0.18f, Cy + R * 0.3f, R * 0.36f, R * 0.55f}, Hex(0x92400e));
		{
			TextStyle T;
			T.Size = R * 0.6f;
			T.Weight = 900;
			T.Col = Hex(0x16a34a);
			T.HAlign = Align::Center;
			T.VAlign = Baseline::Middle;
			C.Text("$", Cx, Cy + R * 0.05f, T);
		}
		break;
	default: break;
	}
}

void Facecam(Canvas& C, const Rect& R, const Cam& L)
{
	const gear::Effects& G = L.Gear;
	const float H = R.H;
	const float Cx = R.X + R.W * 0.5f;
	const double T = L.Time;
	C.PushClip(R);
	// How bright the room is: the monitor's blue glow at night, or real lights.
	const float Light = G.Lights >= 2 ? 1.0f : G.Lights == 1 ? 0.85f : 0.45f;
	const bool Led = L.Leds.On;
	const Color LedCol = Hex(L.Leds.Rgb);
	const float LedLv = Fl(std::max(0.0, std::min(1.6, L.Leds.Level)));
	if (!G.GreenScreen)
	{
		const Color Wall = Led ? Mix(Mix(Hex(0x0f1420), Hex(0x3a3f55), Light * 0.55f), LedCol, 0.18f * std::min(1.0f, LedLv)) : Mix(Hex(0x0f1420), Hex(0x3a3f55), Light * 0.55f);
		C.FillRect(R, Paint::Linear({R.X, R.Y}, {R.X, R.Y + R.H}, Wall, Mix(Wall, Hex(0x000000), 0.5f)));
		// A poster, a shelf, an LED strip in the channel's color.
		C.FillRect({R.X + R.W * 0.08f, R.Y + H * 0.12f, R.W * 0.14f, H * 0.36f}, Mix(Hex(0x27d3c3), Wall, 0.55f));
		C.FillRect({R.X + R.W * 0.1f, R.Y + H * 0.16f, R.W * 0.1f, H * 0.1f}, Mix(Hex(0xf2c14e), Wall, 0.5f));
		C.FillRect({R.X + R.W * 0.7f, R.Y + H * 0.3f, R.W * 0.24f, H * 0.025f}, Mix(Hex(0x8a6a48), Wall, 0.4f));
		C.FillRect({R.X + R.W * 0.74f, R.Y + H * 0.2f, R.W * 0.05f, H * 0.1f}, Mix(Hex(0x22c55e), Wall, 0.5f));
		C.FillRect({R.X + R.W * 0.82f, R.Y + H * 0.22f, R.W * 0.06f, H * 0.08f}, Mix(Hex(0xef4444), Wall, 0.6f));
		if (Led)
		{
			// The room's LEDs: the wall washed in colour from the strip at the top and the one behind the desk.
			C.FillRect({R.X, R.Y, R.W, H * 0.55f}, Paint::Linear({0.0f, R.Y}, {0.0f, R.Y + H * 0.55f}, Alpha(LedCol, 0.5f * std::min(1.2f, LedLv)), Alpha(LedCol, 0.0f)));
			C.FillRect({R.X, R.Y + H * 0.35f, R.W, H * 0.4f}, Paint::Linear({0.0f, R.Y + H * 0.75f}, {0.0f, R.Y + H * 0.35f}, Alpha(LedCol, 0.42f * std::min(1.2f, LedLv)), Alpha(LedCol, 0.0f)));
			LedBar(C, {R.X - 4.0f, R.Y + H * 0.035f, R.W + 8.0f, std::max(2.0f, H * 0.018f)}, LedCol, LedLv);
		}
		else
		{
			C.FillRect({R.X, R.Y + H * 0.04f, R.W, H * 0.015f}, Alpha(Hex(L.Hoodie), 0.6f));
			C.FillRect({R.X, R.Y + H * 0.02f, R.W, H * 0.08f}, Alpha(Hex(L.Hoodie), 0.12f));
		}
		if (G.CamTier >= 3)
		{
			// Shallow depth of field: the room goes soft.
			C.FillRect(R, Rgba(20, 16, 32, 0.35f));
		}
	}
	// Breathing, and the head follows the mood.
	const float Breath = Fl(std::sin(T * 1.6)) * H * 0.006f;
	float Tilt = 0.0f;
	float Lift = 0.0f;
	switch (L.Face)
	{
	case kast::Mood::Laugh: Tilt = -0.08f; Lift = -H * 0.02f; break;
	case kast::Mood::Tilted: Tilt = Fl(std::sin(T * 7.0)) * 0.05f * Fl(std::max(0.0, 1.0 - L.FaceAge / 2.0)); break;
	case kast::Mood::Hyped: Lift = -H * 0.03f * Fl(std::fabs(std::sin(T * 9.0))) * Fl(std::max(0.0, 1.0 - L.FaceAge / 3.0)); break;
	case kast::Mood::Shocked: Lift = -H * 0.015f; break;
	default: Tilt = Fl(std::sin(T * 0.4)) * 0.03f; break;
	}
	const Color Skin = Mix(Hex(0xd9a37c), Hex(0x6d8bd4), (1.0f - Light) * 0.35f);
	const Color SkinDark = Mix(Skin, Hex(0x000000), 0.25f);
	const Color Hoodie = Mix(Hex(L.Hoodie), Hex(0x111827), 0.35f + (1.0f - Light) * 0.3f);
	// Shoulders.
	C.FillEllipse(Cx, R.Y + H * 1.05f + Breath, R.W * 0.3f, H * 0.36f, Paint::Linear({Cx, R.Y + H * 0.7f}, {Cx, R.Y + H}, Mix(Hoodie, Hex(0xffffff), 0.08f), Hoodie));
	C.FillPolygon({{Cx - R.W * 0.05f, R.Y + H * 0.7f}, {Cx + R.W * 0.05f, R.Y + H * 0.7f}, {Cx, R.Y + H * 0.82f}}, Mix(Hoodie, Hex(0x000000), 0.3f));
	// Arms up: the celebration, the hands on the head.
	const bool Arms = (L.Face == kast::Mood::Hyped && L.FaceAge < 3.0) || (L.Face == kast::Mood::Shocked && L.FaceAge < 2.5);
	const float HeadY = R.Y + H * 0.45f + Breath + Lift;
	const float HeadR = H * 0.17f;
	if (Arms)
	{
		const bool Up = L.Face == kast::Mood::Hyped;
		for (int Side = -1; Side <= 1; Side += 2)
		{
			const float Sx = Cx + Fl(Side) * R.W * 0.2f;
			const float Hx = Up ? Cx + Fl(Side) * R.W * 0.26f : Cx + Fl(Side) * HeadR * 0.9f;
			const float Hy = Up ? R.Y + H * 0.12f + Fl(std::sin(T * 10.0 + Side)) * H * 0.02f : HeadY - HeadR * 0.8f;
			C.StrokePolyline({{Sx, R.Y + H * 0.85f}, {Sx + Fl(Side) * R.W * 0.04f, R.Y + H * 0.45f}, {Hx, Hy}}, false, Hoodie, H * 0.1f, true);
			C.FillCircle(Hx, Hy, H * 0.055f, Skin);
		}
	}
	// Head.
	C.Save();
	C.Translate(Cx, HeadY);
	C.Rotate(Tilt);
	C.FillRect({-HeadR * 0.3f, HeadR * 0.6f, HeadR * 0.6f, HeadR * 0.8f}, SkinDark);
	C.FillEllipse(-HeadR * 0.95f, HeadR * 0.05f, HeadR * 0.16f, HeadR * 0.24f, SkinDark);
	C.FillEllipse(HeadR * 0.95f, HeadR * 0.05f, HeadR * 0.16f, HeadR * 0.24f, SkinDark);
	C.FillEllipse(0.0f, 0.0f, HeadR * 0.92f, HeadR * 1.08f, Paint::Radial({-HeadR * 0.3f, -HeadR * 0.4f}, 0.0f, {0.0f, 0.0f}, HeadR * 1.2f, Mix(Skin, Hex(0xffffff), 0.12f * Light), 0.55f, Skin, SkinDark));
	// The LEDs behind: a rim of coloured light around the head.
	if (Led && !G.GreenScreen)
	{
		for (int Side = -1; Side <= 1; Side += 2)
		{
			std::vector<Vec2> Rim;
			for (int I = 0; I <= 10; ++I)
			{
				const float A = -1.25f + 2.1f * Fl(I) / 10.0f; // from the crown down past the cheek
				Rim.push_back({Fl(Side) * HeadR * 0.92f * std::sin(A + 0.35f), -HeadR * 1.08f * std::cos(A + 0.35f)});
			}
			C.StrokePolyline(Rim, false, Alpha(Mix(LedCol, Hex(0xffffff), 0.25f), 0.55f * std::min(1.0f, LedLv)), HeadR * 0.07f, true);
		}
	}
	// Hair: a mop with a beanie-ish swoop.
	C.FillEllipse(0.0f, -HeadR * 0.62f, HeadR * 0.98f, HeadR * 0.55f, Hex(0x2b1d14));
	C.FillPolygon({{-HeadR * 0.95f, -HeadR * 0.5f}, {HeadR * 0.6f, -HeadR * 0.7f}, {HeadR * 0.2f, -HeadR * 0.25f}, {-HeadR * 0.7f, -HeadR * 0.15f}}, Hex(0x2b1d14));
	// Brows and eyes.
	const bool Blink = std::fmod(T, 4.3) < 0.12;
	const bool Wide = L.Face == kast::Mood::Shocked || (L.Face == kast::Mood::Hyped && L.FaceAge < 1.5);
	const float Look = L.Face == kast::Mood::Focus ? HeadR * 0.06f : 0.0f;
	for (int Side = -1; Side <= 1; Side += 2)
	{
		const float Ex = Fl(Side) * HeadR * 0.36f;
		const float Ey = -HeadR * 0.08f;
		if (L.Face == kast::Mood::Laugh || (L.Face == kast::Mood::Happy && L.FaceAge < 2.0) || Blink)
		{
			C.StrokeArc(Ex, Ey + HeadR * 0.05f, HeadR * 0.13f, Pi * 1.15f, Pi * 1.85f, Hex(0x1b1036), HeadR * 0.06f, true);
		}
		else
		{
			C.FillEllipse(Ex, Ey, HeadR * (Wide ? 0.16f : 0.13f), HeadR * (Wide ? 0.17f : 0.1f), Hex(0xf8fafc));
			C.FillCircle(Ex + Look, Ey + (L.Face == kast::Mood::Focus ? HeadR * 0.02f : 0.0f), HeadR * 0.065f, Hex(0x1b1036));
			if (G.Lights == 1)
			{
				C.StrokeEllipse(Ex + Look, Ey, HeadR * 0.04f, HeadR * 0.04f, Rgba(255, 255, 255, 0.85f), HeadR * 0.015f); // the ring light in the eyes
			}
		}
		float BrowTilt = 0.0f;
		float BrowLift = 0.0f;
		switch (L.Face)
		{
		case kast::Mood::Tilted: BrowTilt = 0.35f; break;
		case kast::Mood::Shocked:
		case kast::Mood::Hyped: BrowLift = -HeadR * 0.1f; break;
		case kast::Mood::Happy:
		case kast::Mood::Laugh: BrowLift = -HeadR * 0.05f; break;
		default: break;
		}
		const float Bx0 = Ex - Fl(Side) * HeadR * 0.14f;
		const float Bx1 = Ex + Fl(Side) * HeadR * 0.16f;
		C.StrokePolyline({{Bx0, Ey - HeadR * 0.24f + BrowLift + BrowTilt * HeadR * 0.2f}, {Bx1, Ey - HeadR * 0.26f + BrowLift - BrowTilt * HeadR * 0.12f}}, false, Hex(0x2b1d14), HeadR * 0.07f, true);
	}
	// Mouth.
	const float My = HeadR * 0.5f;
	const bool Talk = L.Talking && std::fmod(T * 8.0, 1.0) < 0.5;
	switch (L.Face)
	{
	case kast::Mood::Hyped:
	case kast::Mood::Laugh: C.FillEllipse(0.0f, My, HeadR * 0.28f, HeadR * (0.16f + 0.04f * Fl(std::fabs(std::sin(T * 12.0)))), Hex(0x4a1020)); break;
	case kast::Mood::Shocked: C.FillEllipse(0.0f, My, HeadR * 0.12f, HeadR * 0.17f, Hex(0x4a1020)); break;
	case kast::Mood::Tilted: C.StrokeArc(0.0f, My + HeadR * 0.16f, HeadR * 0.2f, Pi * 1.2f, Pi * 1.8f, Hex(0x4a1020), HeadR * 0.06f, true); break;
	case kast::Mood::Happy: C.StrokeArc(0.0f, My - HeadR * 0.12f, HeadR * 0.22f, Pi * 0.2f, Pi * 0.8f, Hex(0x4a1020), HeadR * 0.06f, true); break;
	default:
		if (Talk)
		{
			C.FillEllipse(0.0f, My, HeadR * 0.16f, HeadR * 0.09f, Hex(0x4a1020));
		}
		else
		{
			C.StrokePolyline({{-HeadR * 0.14f, My}, {HeadR * 0.14f, My}}, false, Hex(0x4a1020), HeadR * 0.05f, true);
		}
		break;
	}
	// Headphones.
	if (L.Headphones)
	{
		C.StrokeArc(0.0f, -HeadR * 0.05f, HeadR * 1.05f, Pi * 1.05f, Pi * 1.95f, Hex(0x1f2937), HeadR * 0.16f, true);
		for (int Side = -1; Side <= 1; Side += 2)
		{
			C.FillRoundRect({Fl(Side) * HeadR * 1.0f - HeadR * 0.17f, -HeadR * 0.15f, HeadR * 0.34f, HeadR * 0.5f}, HeadR * 0.14f, Hex(0x111827));
			C.FillRoundRect({Fl(Side) * HeadR * 1.0f - HeadR * 0.1f, -HeadR * 0.08f, HeadR * 0.2f, HeadR * 0.36f}, HeadR * 0.08f, Alpha(Hex(L.Hoodie), 0.6f));
		}
	}
	C.Restore();
	// The mic in front.
	if (G.MicTier >= 2)
	{
		C.StrokePolyline({{R.X + R.W * 1.02f, R.Y + H * 0.2f}, {R.X + R.W * 0.78f, R.Y + H * 0.48f}, {R.X + R.W * 0.66f, R.Y + H * 0.64f}}, false, Hex(0x1f2533), H * 0.03f, true);
		C.Save();
		C.Translate(R.X + R.W * 0.63f, R.Y + H * 0.68f);
		C.Rotate(-0.7f);
		C.FillRoundRect({-H * 0.05f, -H * 0.13f, H * 0.1f, H * 0.26f}, H * 0.045f, Paint::Linear({-H * 0.05f, 0.0f}, {H * 0.05f, 0.0f}, Hex(0x374151), Hex(0x0b0f17)));
		C.FillRoundRect({-H * 0.05f, -H * 0.13f, H * 0.1f, H * 0.1f}, H * 0.045f, Hex(0x111827));
		C.Restore();
	}
	else if (G.MicTier == 1)
	{
		const float Mx = R.X + R.W * 0.3f;
		C.FillRect({Mx - H * 0.008f, R.Y + H * 0.82f, H * 0.016f, H * 0.2f}, Hex(0x2a3142));
		C.FillRoundRect({Mx - H * 0.045f, R.Y + H * 0.62f, H * 0.09f, H * 0.22f}, H * 0.045f, Paint::Linear({Mx - H * 0.05f, 0.0f}, {Mx + H * 0.05f, 0.0f}, Hex(0x6b7280), Hex(0x1f2937)));
	}
	// The LED rim on the shoulders, and a little of the colour on everything.
	if (Led && !G.GreenScreen)
	{
		std::vector<Vec2> Rim;
		for (int I = 0; I <= 16; ++I)
		{
			const float A = 3.1416f * (1.08f + 0.84f * Fl(I) / 16.0f);
			Rim.push_back({Cx + std::cos(A) * R.W * 0.3f, R.Y + H * 1.05f + Breath + std::sin(A) * H * 0.36f});
		}
		C.StrokePolyline(Rim, false, Alpha(Mix(LedCol, Hex(0xffffff), 0.2f), 0.5f * std::min(1.0f, LedLv)), H * 0.018f, true);
		C.FillRect(R, Alpha(LedCol, 0.05f * std::min(1.0f, LedLv)));
	}
	// Light: key lights rim the hair; a dark room is dark.
	if (G.Lights >= 2)
	{
		C.FillEllipse(R.X + R.W * 0.15f, R.Y + H * 0.2f, R.W * 0.6f, H * 0.8f, Paint::Radial({R.X + R.W * 0.15f, R.Y + H * 0.2f}, 0.0f, {R.X + R.W * 0.15f, R.Y + H * 0.2f}, R.W * 0.6f, Rgba(255, 236, 200, 0.14f), 0.5f, Rgba(255, 236, 200, 0.05f), Rgba(255, 236, 200, 0.0f)));
	}
	if (G.Lights == 0)
	{
		// The LEDs light the room a little even without a light on the face.
		C.FillRect(R, Rgba(4, 8, 20, Led ? 0.2f : 0.32f));
		C.FillRect(R, Paint::Linear({R.X, R.Y + H}, {R.X, R.Y}, Rgba(80, 140, 255, Led ? 0.05f : 0.12f), Rgba(80, 140, 255, 0.0f)));
	}
	if (L.Face == kast::Mood::Tilted)
	{
		Vignette(C, R, Rgba(170, 0, 0, 0.16f), 0.4f);
	}
	// Sensor noise from a cheap camera.
	if (G.CamTier <= 1)
	{
		const int Count = G.CamTier == 0 ? 90 : 30;
		const uint32_t Frame = static_cast<uint32_t>(T * 24.0);
		for (int K = 0; K < Count; ++K)
		{
			uint32_t Hh = (Frame * 2654435761u) ^ (static_cast<uint32_t>(K) * 40503u);
			Hh ^= Hh >> 13;
			Hh *= 0x5bd1e995u;
			Hh ^= Hh >> 15;
			const float U = Fl(Hh & 0xffff) / 65535.0f;
			const float V = Fl((Hh >> 16) & 0xffff) / 65535.0f;
			const float Sz = std::max(1.0f, H * (G.CamTier == 0 ? 0.012f : 0.007f));
			C.FillRect({R.X + U * R.W, R.Y + V * H, Sz, Sz}, Rgba(255, 255, 255, G.CamTier == 0 ? 0.14f : 0.08f));
		}
		if (G.CamTier == 0)
		{
			C.FillRect(R, Rgba(60, 60, 70, 0.18f)); // washed out
		}
	}
	// Vignette.
	if (!G.GreenScreen)
	{
		Vignette(C, R, Rgba(0, 0, 0, 0.32f), 0.22f);
	}
	C.PopClip();
}
} // namespace streamart
} // namespace ui
} // namespace ss
