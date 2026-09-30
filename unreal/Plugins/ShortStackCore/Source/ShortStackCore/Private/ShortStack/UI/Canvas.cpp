#include "ShortStack/UI/Canvas.h"
#include "../StrictFloat.h"

#include <cmath>
#include <algorithm>
#include <cstdio>

namespace ss
{
namespace ui
{
namespace canvas_detail
{
const float Pi = 3.14159265358979f;

Vec2 Sub(Vec2 A, Vec2 B) { return {A.X - B.X, A.Y - B.Y}; }
Vec2 Add(Vec2 A, Vec2 B) { return {A.X + B.X, A.Y + B.Y}; }
Vec2 Mul(Vec2 A, float S) { return {A.X * S, A.Y * S}; }
float Dot(Vec2 A, Vec2 B) { return A.X * B.X + A.Y * B.Y; }
float Cross(Vec2 A, Vec2 B) { return A.X * B.Y - A.Y * B.X; }
float Len(Vec2 A) { return std::sqrt(A.X * A.X + A.Y * A.Y); }
Vec2 Norm(Vec2 A)
{
	const float L = Len(A);
	return L > 1e-9f ? Vec2{A.X / L, A.Y / L} : Vec2{0.0f, 0.0f};
}
float Clampf(float V, float Lo, float Hi) { return V < Lo ? Lo : V > Hi ? Hi : V; }

float SignedArea(const std::vector<Vec2>& P)
{
	float A = 0.0f;
	for (size_t I = 0; I < P.size(); ++I)
	{
		const Vec2& P0 = P[I];
		const Vec2& P1 = P[(I + 1) % P.size()];
		A += P0.X * P1.Y - P1.X * P0.Y;
	}
	return A * 0.5f;
}

bool IsConvex(const std::vector<Vec2>& P)
{
	const size_t N = P.size();
	int Sign = 0;
	for (size_t I = 0; I < N; ++I)
	{
		const float C = Cross(Sub(P[(I + 1) % N], P[I]), Sub(P[(I + 2) % N], P[(I + 1) % N]));
		if (std::fabs(C) < 1e-7f)
		{
			continue;
		}
		const int S = C > 0.0f ? 1 : -1;
		if (Sign != 0 && S != Sign)
		{
			return false;
		}
		Sign = S;
	}
	return true;
}

bool PointInTri(Vec2 P, Vec2 A, Vec2 B, Vec2 C)
{
	const float D1 = Cross(Sub(B, A), Sub(P, A));
	const float D2 = Cross(Sub(C, B), Sub(P, B));
	const float D3 = Cross(Sub(A, C), Sub(P, C));
	const bool Neg = D1 < 0.0f || D2 < 0.0f || D3 < 0.0f;
	const bool Pos = D1 > 0.0f || D2 > 0.0f || D3 > 0.0f;
	return !(Neg && Pos);
}

/** Ear clipping for a simple polygon with positive area; returns index triples. */
std::vector<uint32_t> EarClip(const std::vector<Vec2>& P)
{
	std::vector<uint32_t> Tris;
	std::vector<uint32_t> Idx;
	for (uint32_t I = 0; I < P.size(); ++I)
	{
		Idx.push_back(I);
	}
	int Guard = 0;
	while (Idx.size() > 3 && Guard++ < 10000)
	{
		bool Clipped = false;
		const size_t N = Idx.size();
		for (size_t I = 0; I < N; ++I)
		{
			const uint32_t Ia = Idx[(I + N - 1) % N];
			const uint32_t Ib = Idx[I];
			const uint32_t Ic = Idx[(I + 1) % N];
			const Vec2 A = P[Ia];
			const Vec2 B = P[Ib];
			const Vec2 C = P[Ic];
			if (Cross(Sub(B, A), Sub(C, B)) <= 0.0f)
			{
				continue; // reflex vertex
			}
			bool Inside = false;
			for (size_t J = 0; J < N && !Inside; ++J)
			{
				const uint32_t Ij = Idx[J];
				if (Ij != Ia && Ij != Ib && Ij != Ic)
				{
					Inside = PointInTri(P[Ij], A, B, C);
				}
			}
			if (Inside)
			{
				continue;
			}
			Tris.push_back(Ia);
			Tris.push_back(Ib);
			Tris.push_back(Ic);
			Idx.erase(Idx.begin() + static_cast<long>(I));
			Clipped = true;
			break;
		}
		if (!Clipped)
		{
			break; // degenerate input: fan the rest
		}
	}
	for (size_t I = 1; I + 1 < Idx.size(); ++I)
	{
		Tris.push_back(Idx[0]);
		Tris.push_back(Idx[I]);
		Tris.push_back(Idx[I + 1]);
	}
	return Tris;
}

void AppendJsonString(std::string& Out, const std::string& S)
{
	Out.push_back('"');
	for (const char Ch : S)
	{
		if (Ch == '"' || Ch == '\\')
		{
			Out.push_back('\\');
			Out.push_back(Ch);
		}
		else if (static_cast<unsigned char>(Ch) < 0x20)
		{
			Out += ' ';
		}
		else
		{
			Out.push_back(Ch);
		}
	}
	Out.push_back('"');
}

void AppendNum(std::string& Out, float V)
{
	char Buf[32];
	std::snprintf(Buf, sizeof(Buf), "%.2f", static_cast<double>(V));
	Out += Buf;
}

void PopCodepoint(std::string& S)
{
	while (!S.empty() && (static_cast<unsigned char>(S.back()) & 0xC0) == 0x80)
	{
		S.pop_back();
	}
	if (!S.empty())
	{
		S.pop_back();
	}
}
} // namespace canvas_detail

using namespace canvas_detail;

// ------------------------------------------------------------------ color, transform, paint

Color Mix(const Color& A, const Color& B, float T)
{
	return {A.R + (B.R - A.R) * T, A.G + (B.G - A.G) * T, A.B + (B.B - A.B) * T, A.A + (B.A - A.A) * T};
}

Color Hsl(float HueDeg, float Sat, float Light, float A)
{
	const float H = std::fmod(HueDeg, 360.0f) / 360.0f;
	auto Channel = [&](float N) {
		const float K = std::fmod(N + H * 12.0f, 12.0f);
		const float Amp = Sat * (Light < 1.0f - Light ? Light : 1.0f - Light);
		const float M = Clampf(K - 3.0f < 9.0f - K ? K - 3.0f : 9.0f - K, -1.0f, 1.0f);
		return Light - Amp * M;
	};
	return {Channel(0.0f), Channel(8.0f), Channel(4.0f), A};
}

Affine Affine::Then(const Affine& O) const
{
	Affine R;
	R.A = A * O.A + C * O.B;
	R.B = B * O.A + D * O.B;
	R.C = A * O.C + C * O.D;
	R.D = B * O.C + D * O.D;
	R.Tx = A * O.Tx + C * O.Ty + Tx;
	R.Ty = B * O.Tx + D * O.Ty + Ty;
	return R;
}

float Affine::UniformScale() const
{
	return std::sqrt(std::fabs(A * D - B * C));
}

bool Affine::IsUniform(float Tolerance) const
{
	const float SX = std::sqrt(A * A + B * B);
	const float SY = std::sqrt(C * C + D * D);
	const float M = SX > SY ? SX : SY;
	return M > 0.0f && std::fabs(SX - SY) / M < Tolerance;
}

Paint Paint::Linear(Vec2 From, Vec2 To, const Color& A, const Color& B)
{
	Paint P;
	P.Type = Kind::Linear;
	P.P0 = From;
	P.P1 = To;
	P.C0 = A;
	P.C1 = B;
	P.C2 = B;
	return P;
}

Paint Paint::Radial(Vec2 C0Center, float Radius0, Vec2 C1Center, float Radius1, const Color& A, float MidT, const Color& MidC, const Color& B)
{
	Paint P;
	P.Type = Kind::Radial;
	P.P0 = C0Center;
	P.R0 = Radius0;
	P.P1 = C1Center;
	P.R1 = Radius1;
	P.C0 = A;
	if (MidT < 0.0f)
	{
		P.C1 = B;
		P.C2 = B;
	}
	else
	{
		P.Mid = MidT;
		P.C1 = MidC;
		P.C2 = B;
	}
	return P;
}

Color Paint::At(Vec2 P) const
{
	float T = 0.0f;
	if (Type == Kind::Solid)
	{
		return C0;
	}
	if (Type == Kind::Linear)
	{
		const Vec2 D = Sub(P1, P0);
		const float L2 = Dot(D, D);
		T = L2 > 0.0f ? Dot(Sub(P, P0), D) / L2 : 0.0f;
	}
	else
	{
		// Two-circle radial gradient (canvas createRadialGradient): largest t with |P - c(t)| = r(t).
		const Vec2 Cd = Sub(P1, P0);
		const float Dr = R1 - R0;
		const Vec2 Pd = Sub(P, P0);
		const float Qa = Dot(Cd, Cd) - Dr * Dr;
		const float Qb = Dot(Pd, Cd) + R0 * Dr;
		const float Qc = Dot(Pd, Pd) - R0 * R0;
		if (std::fabs(Qa) < 1e-6f)
		{
			T = std::fabs(Qb) > 1e-9f ? Qc / (2.0f * Qb) : 0.0f;
		}
		else
		{
			const float Disc = Qb * Qb - Qa * Qc;
			T = Disc >= 0.0f ? (Qb + std::sqrt(Disc)) / Qa : 0.0f;
		}
	}
	T = Clampf(T, 0.0f, 1.0f);
	if (Mid < 0.0f)
	{
		return Mix(C0, C1, T);
	}
	return T < Mid ? Mix(C0, C1, Mid > 0.0f ? T / Mid : 1.0f) : Mix(C1, C2, Mid < 1.0f ? (T - Mid) / (1.0f - Mid) : 1.0f);
}

// ------------------------------------------------------------------ draw list

void DrawList::Clear()
{
	Vertices.clear();
	Indices.clear();
	Cmds.clear();
}

uint32_t DrawList::BeginTriangles(size_t ExtraVertices)
{
	if (Cmds.empty() || Cmds.back().Type != DrawCmd::Kind::Triangles || Cmds.back().VertexCount + ExtraVertices > 60000)
	{
		DrawCmd C;
		C.Type = DrawCmd::Kind::Triangles;
		C.VertexStart = static_cast<uint32_t>(Vertices.size());
		C.IndexStart = static_cast<uint32_t>(Indices.size());
		Cmds.push_back(C);
	}
	return Cmds.back().VertexCount;
}

void DrawList::AddVertex(float X, float Y, const Color& C)
{
	Vertex V;
	V.X = X;
	V.Y = Y;
	V.Col = C;
	Vertices.push_back(V);
	++Cmds.back().VertexCount;
}

void DrawList::AddTriangle(uint32_t I0, uint32_t I1, uint32_t I2)
{
	Indices.push_back(I0);
	Indices.push_back(I1);
	Indices.push_back(I2);
	Cmds.back().IndexCount += 3;
}

void DrawList::AddText(const TextItem& Item)
{
	DrawCmd C;
	C.Type = DrawCmd::Kind::Text;
	C.Text = Item;
	Cmds.push_back(C);
}

void DrawList::PushClip(const Rect& R)
{
	DrawCmd C;
	C.Type = DrawCmd::Kind::PushClip;
	C.Clip = R;
	Cmds.push_back(C);
}

void DrawList::PopClip()
{
	DrawCmd C;
	C.Type = DrawCmd::Kind::PopClip;
	Cmds.push_back(C);
}

std::string DrawList::ToJson() const
{
	std::string Out = "{\"w\":";
	AppendNum(Out, Width);
	Out += ",\"h\":";
	AppendNum(Out, Height);
	Out += ",\"cmds\":[";
	for (size_t K = 0; K < Cmds.size(); ++K)
	{
		const DrawCmd& C = Cmds[K];
		if (K > 0)
		{
			Out += ',';
		}
		if (C.Type == DrawCmd::Kind::Triangles)
		{
			Out += "{\"t\":\"tri\",\"v\":[";
			for (uint32_t I = 0; I < C.VertexCount; ++I)
			{
				const Vertex& V = Vertices[C.VertexStart + I];
				if (I > 0)
				{
					Out += ',';
				}
				AppendNum(Out, V.X);
				Out += ',';
				AppendNum(Out, V.Y);
				char Buf[64];
				std::snprintf(Buf, sizeof(Buf), ",%d,%d,%d,%.3f", static_cast<int>(V.Col.R * 255.0f + 0.5f), static_cast<int>(V.Col.G * 255.0f + 0.5f), static_cast<int>(V.Col.B * 255.0f + 0.5f), static_cast<double>(V.Col.A));
				Out += Buf;
			}
			Out += "],\"i\":[";
			for (uint32_t I = 0; I < C.IndexCount; ++I)
			{
				if (I > 0)
				{
					Out += ',';
				}
				Out += std::to_string(Indices[C.IndexStart + I]);
			}
			Out += "]}";
		}
		else if (C.Type == DrawCmd::Kind::Text)
		{
			const TextItem& T = C.Text;
			Out += "{\"t\":\"text\",\"s\":";
			AppendJsonString(Out, T.Text);
			Out += ",\"x\":";
			AppendNum(Out, T.X);
			Out += ",\"y\":";
			AppendNum(Out, T.BaselineY);
			Out += ",\"size\":";
			AppendNum(Out, T.SizePx);
			Out += ",\"font\":";
			Out += std::to_string(static_cast<int>(T.Face));
			char Buf[64];
			std::snprintf(Buf, sizeof(Buf), ",\"c\":[%d,%d,%d,%.3f]}", static_cast<int>(T.Col.R * 255.0f + 0.5f), static_cast<int>(T.Col.G * 255.0f + 0.5f), static_cast<int>(T.Col.B * 255.0f + 0.5f), static_cast<double>(T.Col.A));
			Out += Buf;
		}
		else if (C.Type == DrawCmd::Kind::PushClip)
		{
			Out += "{\"t\":\"clip\",\"r\":[";
			AppendNum(Out, C.Clip.X);
			Out += ',';
			AppendNum(Out, C.Clip.Y);
			Out += ',';
			AppendNum(Out, C.Clip.W);
			Out += ',';
			AppendNum(Out, C.Clip.H);
			Out += "]}";
		}
		else
		{
			Out += "{\"t\":\"pop\"}";
		}
	}
	Out += "]}";
	return Out;
}

// ------------------------------------------------------------------ canvas state

Canvas::Canvas(DrawList& InOut, const TextMeasurer& InMeasurer, float Width, float Height, float PixelScale)
	: Out(InOut), Measurer(InMeasurer), W(Width), H(Height), PxScale(PixelScale > 0.01f ? PixelScale : 1.0f)
{
	Out.Width = Width;
	Out.Height = Height;
}

void Canvas::Save()
{
	Stack.push_back(State);
}

void Canvas::Restore()
{
	if (!Stack.empty())
	{
		State = Stack.back();
		Stack.pop_back();
	}
}

void Canvas::Translate(float X, float Y)
{
	Affine M;
	M.Tx = X;
	M.Ty = Y;
	State.Xf = State.Xf.Then(M);
}

void Canvas::Scale(float SX, float SY)
{
	Affine M;
	M.A = SX;
	M.D = SY;
	State.Xf = State.Xf.Then(M);
}

void Canvas::Rotate(float Radians)
{
	Affine M;
	M.A = std::cos(Radians);
	M.B = std::sin(Radians);
	M.C = -M.B;
	M.D = M.A;
	State.Xf = State.Xf.Then(M);
}

void Canvas::PushClip(const Rect& R)
{
	const Vec2 P[4] = {State.Xf.Apply({R.X, R.Y}), State.Xf.Apply({R.X + R.W, R.Y}), State.Xf.Apply({R.X, R.Y + R.H}), State.Xf.Apply({R.X + R.W, R.Y + R.H})};
	float X0 = P[0].X, Y0 = P[0].Y, X1 = P[0].X, Y1 = P[0].Y;
	for (const Vec2& Q : P)
	{
		X0 = Q.X < X0 ? Q.X : X0;
		Y0 = Q.Y < Y0 ? Q.Y : Y0;
		X1 = Q.X > X1 ? Q.X : X1;
		Y1 = Q.Y > Y1 ? Q.Y : Y1;
	}
	if (!Clips.empty())
	{
		const Rect& C = Clips.back();
		X0 = X0 > C.X ? X0 : C.X;
		Y0 = Y0 > C.Y ? Y0 : C.Y;
		X1 = X1 < C.X + C.W ? X1 : C.X + C.W;
		Y1 = Y1 < C.Y + C.H ? Y1 : C.Y + C.H;
	}
	const Rect Clip{X0, Y0, X1 > X0 ? X1 - X0 : 0.0f, Y1 > Y0 ? Y1 - Y0 : 0.0f};
	Clips.push_back(Clip);
	Out.PushClip(Clip);
}

void Canvas::PopClip()
{
	if (!Clips.empty())
	{
		Clips.pop_back();
		Out.PopClip();
	}
}

int Canvas::SegmentsFor(float RadiusLogical, float ArcRadians) const
{
	const float Rd = RadiusLogical * State.Xf.UniformScale() * PxScale;
	const float N = std::ceil(std::fabs(ArcRadians) * std::sqrt(Rd > 0.0f ? Rd : 0.0f) * 0.9f);
	return static_cast<int>(Clampf(N, 2.0f, 160.0f));
}

std::vector<Vec2> Canvas::RoundRectPath(const Rect& R, float Radius, int Segs)
{
	std::vector<Vec2> P;
	const float MaxR = (R.W < R.H ? R.W : R.H) * 0.5f;
	const float Rad = Clampf(Radius, 0.0f, MaxR);
	if (Rad < 0.01f)
	{
		P.push_back({R.X, R.Y});
		P.push_back({R.X + R.W, R.Y});
		P.push_back({R.X + R.W, R.Y + R.H});
		P.push_back({R.X, R.Y + R.H});
		return P;
	}
	const Vec2 Centers[4] = {{R.X + Rad, R.Y + Rad}, {R.X + R.W - Rad, R.Y + Rad}, {R.X + R.W - Rad, R.Y + R.H - Rad}, {R.X + Rad, R.Y + R.H - Rad}};
	const float Starts[4] = {Pi, Pi * 1.5f, 0.0f, Pi * 0.5f};
	for (int C = 0; C < 4; ++C)
	{
		for (int I = 0; I <= Segs; ++I)
		{
			const float A = Starts[C] + (Pi * 0.5f) * static_cast<float>(I) / static_cast<float>(Segs);
			P.push_back({Centers[C].X + Rad * std::cos(A), Centers[C].Y + Rad * std::sin(A)});
		}
	}
	return P;
}

std::vector<Vec2> Canvas::EllipsePath(float CX, float CY, float RX, float RY, float A0, float A1, int Segments, bool IncludeEnd)
{
	std::vector<Vec2> P;
	const int N = Segments < 2 ? 2 : Segments;
	for (int I = 0; I < N + (IncludeEnd ? 1 : 0); ++I)
	{
		const float A = A0 + (A1 - A0) * static_cast<float>(I) / static_cast<float>(N);
		P.push_back({CX + RX * std::cos(A), CY + RY * std::sin(A)});
	}
	return P;
}

// ------------------------------------------------------------------ tessellation

void Canvas::FillPath(const std::vector<Vec2>& LogicalIn, const Paint& P, float FeatherUnits, bool Convex)
{
	if (LogicalIn.size() < 3 || State.Alpha <= 0.001f)
	{
		return;
	}
	std::vector<Vec2> Logical = LogicalIn;
	std::vector<Vec2> Dev;
	Dev.reserve(Logical.size());
	for (const Vec2& L : Logical)
	{
		Dev.push_back(State.Xf.Apply(L));
	}
	const float Area = SignedArea(Dev);
	if (std::fabs(Area) < 1e-6f)
	{
		return;
	}
	if (Area < 0.0f)
	{
		std::reverse(Logical.begin(), Logical.end());
		std::reverse(Dev.begin(), Dev.end());
	}
	const size_t N = Dev.size();
	const bool IsConvexShape = Convex || IsConvex(Dev);
	const uint32_t Base = Out.BeginTriangles(N * 2 + 1);
	for (size_t I = 0; I < N; ++I)
	{
		Out.AddVertex(Dev[I].X, Dev[I].Y, Faded(P.At(Logical[I])));
	}
	if (IsConvexShape)
	{
		Vec2 CL{0.0f, 0.0f};
		for (const Vec2& L : Logical)
		{
			CL = Add(CL, L);
		}
		CL = Mul(CL, 1.0f / static_cast<float>(N));
		const Vec2 CD = State.Xf.Apply(CL);
		Out.AddVertex(CD.X, CD.Y, Faded(P.At(CL)));
		const uint32_t Center = Base + static_cast<uint32_t>(N);
		for (size_t I = 0; I < N; ++I)
		{
			Out.AddTriangle(Center, Base + static_cast<uint32_t>(I), Base + static_cast<uint32_t>((I + 1) % N));
		}
	}
	else
	{
		const std::vector<uint32_t> Tris = EarClip(Dev);
		for (size_t I = 0; I + 2 < Tris.size(); I += 3)
		{
			Out.AddTriangle(Base + Tris[I], Base + Tris[I + 1], Base + Tris[I + 2]);
		}
		Out.AddVertex(Dev[0].X, Dev[0].Y, Color{0, 0, 0, 0}); // keeps the vertex layout identical to the convex path
	}
	// Anti-aliasing fringe: an outer ring fading to transparent.
	const uint32_t Ring = Base + static_cast<uint32_t>(N) + 1;
	for (size_t I = 0; I < N; ++I)
	{
		const Vec2 Prev = Dev[(I + N - 1) % N];
		const Vec2 Cur = Dev[I];
		const Vec2 Next = Dev[(I + 1) % N];
		const Vec2 E0 = Norm(Sub(Cur, Prev));
		const Vec2 E1 = Norm(Sub(Next, Cur));
		const Vec2 N0{E0.Y, -E0.X};
		const Vec2 N1{E1.Y, -E1.X};
		Vec2 Nm = Norm(Add(N0, N1));
		if (Len(Nm) < 0.5f)
		{
			Nm = N1;
		}
		const float Miter = Clampf(1.0f / (Dot(Nm, N1) > 0.3f ? Dot(Nm, N1) : 0.3f), 1.0f, 3.0f);
		const Vec2 O = Add(Cur, Mul(Nm, FeatherUnits * Miter));
		Color C = Faded(P.At(Logical[I]));
		C.A = 0.0f;
		Out.AddVertex(O.X, O.Y, C);
	}
	for (size_t I = 0; I < N; ++I)
	{
		const uint32_t A = Base + static_cast<uint32_t>(I);
		const uint32_t B = Base + static_cast<uint32_t>((I + 1) % N);
		const uint32_t OA = Ring + static_cast<uint32_t>(I);
		const uint32_t OB = Ring + static_cast<uint32_t>((I + 1) % N);
		Out.AddTriangle(A, B, OB);
		Out.AddTriangle(A, OB, OA);
	}
}

void Canvas::FillRings(float CX, float CY, float RX, float RY, const Paint& P)
{
	const int Segs = SegmentsFor(RX > RY ? RX : RY, 2.0f * Pi);
	const int Rings = 6;
	const size_t Count = 1 + static_cast<size_t>(Rings * Segs) + static_cast<size_t>(Segs);
	const uint32_t Base = Out.BeginTriangles(Count);
	const Vec2 CL{CX, CY};
	const Vec2 CD = State.Xf.Apply(CL);
	Out.AddVertex(CD.X, CD.Y, Faded(P.At(CL)));
	std::vector<Vec2> Outer;
	std::vector<Color> OuterCol;
	for (int K = 1; K <= Rings; ++K)
	{
		const float T = static_cast<float>(K) / static_cast<float>(Rings);
		for (int I = 0; I < Segs; ++I)
		{
			const float A = 2.0f * Pi * static_cast<float>(I) / static_cast<float>(Segs);
			const Vec2 L{CX + RX * T * std::cos(A), CY + RY * T * std::sin(A)};
			const Vec2 D = State.Xf.Apply(L);
			const Color C = Faded(P.At(L));
			Out.AddVertex(D.X, D.Y, C);
			if (K == Rings)
			{
				Outer.push_back(D);
				OuterCol.push_back(C);
			}
		}
	}
	const uint32_t S = static_cast<uint32_t>(Segs);
	for (uint32_t I = 0; I < S; ++I)
	{
		Out.AddTriangle(Base, Base + 1 + I, Base + 1 + (I + 1) % S);
	}
	for (uint32_t K = 1; K < static_cast<uint32_t>(Rings); ++K)
	{
		const uint32_t In = Base + 1 + (K - 1) * S;
		const uint32_t Ou = Base + 1 + K * S;
		for (uint32_t I = 0; I < S; ++I)
		{
			const uint32_t J = (I + 1) % S;
			Out.AddTriangle(In + I, Ou + I, Ou + J);
			Out.AddTriangle(In + I, Ou + J, In + J);
		}
	}
	// Fringe (orientation independent: push away from the center).
	const uint32_t Last = Base + 1 + static_cast<uint32_t>(Rings - 1) * S;
	const uint32_t Ring = Base + 1 + static_cast<uint32_t>(Rings) * S;
	for (size_t I = 0; I < Outer.size(); ++I)
	{
		const Vec2 Dir = Norm(Sub(Outer[I], CD));
		const Vec2 O = Add(Outer[I], Mul(Dir, Feather()));
		Color C = OuterCol[I];
		C.A = 0.0f;
		Out.AddVertex(O.X, O.Y, C);
	}
	for (uint32_t I = 0; I < S; ++I)
	{
		const uint32_t J = (I + 1) % S;
		Out.AddTriangle(Last + I, Ring + I, Ring + J);
		Out.AddTriangle(Last + I, Ring + J, Last + J);
	}
}

void Canvas::StrokeDevice(const std::vector<Vec2>& DevIn, bool Closed, const Color& CIn, float WidthUnits, bool ExtendCaps)
{
	std::vector<Vec2> Dev;
	for (const Vec2& P : DevIn)
	{
		if (Dev.empty() || Len(Sub(P, Dev.back())) > 1e-4f)
		{
			Dev.push_back(P);
		}
	}
	if (Closed && Dev.size() > 2 && Len(Sub(Dev.front(), Dev.back())) < 1e-4f)
	{
		Dev.pop_back();
	}
	if (Dev.size() < 2 || State.Alpha <= 0.001f)
	{
		return;
	}
	Color C = Faded(CIn);
	float Width = WidthUnits;
	const float F = Feather();
	if (Width < F)
	{
		C.A *= Width / F;
		Width = F;
	}
	const float Half = Width * 0.5f;
	const size_t N = Dev.size();
	if (!Closed && ExtendCaps)
	{
		Dev[0] = Sub(Dev[0], Mul(Norm(Sub(Dev[1], Dev[0])), Half));
		Dev[N - 1] = Add(Dev[N - 1], Mul(Norm(Sub(Dev[N - 1], Dev[N - 2])), Half));
	}
	const uint32_t Base = Out.BeginTriangles(N * 4);
	Color Clear = C;
	Clear.A = 0.0f;
	for (size_t I = 0; I < N; ++I)
	{
		Vec2 Nm;
		float Miter = 1.0f;
		const bool HasPrev = Closed || I > 0;
		const bool HasNext = Closed || I + 1 < N;
		const Vec2 Cur = Dev[I];
		Vec2 N0{0.0f, 0.0f};
		Vec2 N1{0.0f, 0.0f};
		if (HasPrev)
		{
			const Vec2 E = Norm(Sub(Cur, Dev[(I + N - 1) % N]));
			N0 = {-E.Y, E.X};
		}
		if (HasNext)
		{
			const Vec2 E = Norm(Sub(Dev[(I + 1) % N], Cur));
			N1 = {-E.Y, E.X};
		}
		if (HasPrev && HasNext)
		{
			Nm = Norm(Add(N0, N1));
			if (Len(Nm) < 0.5f)
			{
				Nm = N1;
			}
			const float D = Dot(Nm, N1);
			Miter = Clampf(1.0f / (D > 0.25f ? D : 0.25f), 1.0f, 4.0f);
		}
		else
		{
			Nm = HasPrev ? N0 : N1;
		}
		const float Inner = Half * Miter;
		const float Outer = (Half + F) * Miter;
		Out.AddVertex(Cur.X + Nm.X * Outer, Cur.Y + Nm.Y * Outer, Clear);
		Out.AddVertex(Cur.X + Nm.X * Inner, Cur.Y + Nm.Y * Inner, C);
		Out.AddVertex(Cur.X - Nm.X * Inner, Cur.Y - Nm.Y * Inner, C);
		Out.AddVertex(Cur.X - Nm.X * Outer, Cur.Y - Nm.Y * Outer, Clear);
	}
	const size_t Segs = Closed ? N : N - 1;
	for (size_t I = 0; I < Segs; ++I)
	{
		const uint32_t A = Base + static_cast<uint32_t>(I * 4);
		const uint32_t B = Base + static_cast<uint32_t>(((I + 1) % N) * 4);
		for (uint32_t K = 0; K < 3; ++K)
		{
			Out.AddTriangle(A + K, B + K, B + K + 1);
			Out.AddTriangle(A + K, B + K + 1, A + K + 1);
		}
	}
}

// ------------------------------------------------------------------ shapes

void Canvas::FillRect(const Rect& R, const Paint& P)
{
	FillPath({{R.X, R.Y}, {R.X + R.W, R.Y}, {R.X + R.W, R.Y + R.H}, {R.X, R.Y + R.H}}, P, Feather(), true);
}

void Canvas::FillRoundRect(const Rect& R, float Radius, const Paint& P)
{
	if (R.W <= 0.0f || R.H <= 0.0f)
	{
		return;
	}
	FillPath(RoundRectPath(R, Radius, SegmentsFor(Radius, Pi * 0.5f)), P, Feather(), true);
}

void Canvas::StrokeRoundRect(const Rect& R, float Radius, const Color& C, float LineWidth)
{
	if (R.W <= 0.0f || R.H <= 0.0f)
	{
		return;
	}
	const std::vector<Vec2> L = RoundRectPath(R, Radius, SegmentsFor(Radius, Pi * 0.5f));
	std::vector<Vec2> D;
	for (const Vec2& P : L)
	{
		D.push_back(State.Xf.Apply(P));
	}
	StrokeDevice(D, true, C, LineWidth * State.Xf.UniformScale(), false);
}

void Canvas::FillEllipse(float CX, float CY, float RX, float RY, const Paint& P)
{
	if (RX <= 0.0f || RY <= 0.0f)
	{
		return;
	}
	if (P.Type == Paint::Kind::Radial)
	{
		FillRings(CX, CY, RX, RY, P);
		return;
	}
	FillPath(EllipsePath(CX, CY, RX, RY, 0.0f, 2.0f * Pi, SegmentsFor(RX > RY ? RX : RY, 2.0f * Pi), false), P, Feather(), true);
}

void Canvas::StrokeEllipse(float CX, float CY, float RX, float RY, const Color& C, float LineWidth, float Dash)
{
	const std::vector<Vec2> L = EllipsePath(CX, CY, RX, RY, 0.0f, 2.0f * Pi, SegmentsFor(RX > RY ? RX : RY, 2.0f * Pi) * (Dash > 0.0f ? 2 : 1), false);
	std::vector<Vec2> D;
	for (const Vec2& P : L)
	{
		D.push_back(State.Xf.Apply(P));
	}
	const float Wd = LineWidth * State.Xf.UniformScale();
	if (Dash <= 0.0f)
	{
		StrokeDevice(D, true, C, Wd, false);
		return;
	}
	// Dashed: walk the outline, alternating on/off every Dash units.
	const float DashD = Dash * State.Xf.UniformScale();
	D.push_back(D.front());
	std::vector<Vec2> Seg;
	float Along = 0.0f;
	bool On = true;
	Seg.push_back(D[0]);
	for (size_t I = 1; I < D.size(); ++I)
	{
		Vec2 A = D[I - 1];
		const Vec2 B = D[I];
		float Remaining = Len(Sub(B, A));
		while (Remaining > 0.0f)
		{
			const float Step = DashD - Along;
			if (Step <= Remaining)
			{
				const Vec2 Dir = Norm(Sub(B, A));
				A = Add(A, Mul(Dir, Step));
				Remaining -= Step;
				if (On)
				{
					Seg.push_back(A);
					StrokeDevice(Seg, false, C, Wd, false);
				}
				Seg.clear();
				Seg.push_back(A);
				On = !On;
				Along = 0.0f;
			}
			else
			{
				Along += Remaining;
				Remaining = 0.0f;
				if (On)
				{
					Seg.push_back(B);
				}
			}
		}
	}
	if (On && Seg.size() > 1)
	{
		StrokeDevice(Seg, false, C, Wd, false);
	}
}

void Canvas::StrokeArc(float CX, float CY, float R, float A0, float A1, const Color& C, float LineWidth, bool RoundCap)
{
	const std::vector<Vec2> L = EllipsePath(CX, CY, R, R, A0, A1, SegmentsFor(R, A1 - A0), true);
	std::vector<Vec2> D;
	for (const Vec2& P : L)
	{
		D.push_back(State.Xf.Apply(P));
	}
	StrokeDevice(D, false, C, LineWidth * State.Xf.UniformScale(), RoundCap);
}

void Canvas::FillPolygon(const std::vector<Vec2>& Points, const Paint& P)
{
	FillPath(Points, P, Feather(), false);
}

void Canvas::StrokePolyline(const std::vector<Vec2>& Points, bool Closed, const Color& C, float LineWidth, bool RoundCap)
{
	std::vector<Vec2> D;
	for (const Vec2& P : Points)
	{
		D.push_back(State.Xf.Apply(P));
	}
	StrokeDevice(D, Closed, C, LineWidth * State.Xf.UniformScale(), RoundCap);
}

void Canvas::GlowRoundRect(const Rect& R, float Radius, const Color& C, float Blur)
{
	// A gaussian-ish falloff: a solid core inset by a third of the blur, then a wide fringe to transparent.
	const float Inset = Blur * 0.3f;
	const Rect Core{R.X + Inset, R.Y + Inset, R.W - Inset * 2.0f, R.H - Inset * 2.0f};
	if (Core.W <= 0.0f || Core.H <= 0.0f)
	{
		return;
	}
	const float Rad = Radius > Inset ? Radius - Inset : 0.0f;
	FillPath(RoundRectPath(Core, Rad, SegmentsFor(Rad + Blur, Pi * 0.5f)), Paint(C), Blur * 1.3f * State.Xf.UniformScale(), true);
}

// ------------------------------------------------------------------ text

float Canvas::Measure(const std::string& S, float Size, int Weight, bool Mono) const
{
	return Measurer.Width(S, FaceFor(Weight, Mono), Size);
}

float Canvas::Text(const std::string& S, float X, float Y, const TextStyle& Style)
{
	const Font Face = FaceFor(Style.Weight, Style.Mono);
	std::string Str = S;
	if (Style.MaxWidth > 0.0f && Measurer.Width(Str, Face, Style.Size) > Style.MaxWidth)
	{
		const std::string Ellipsis = "\xE2\x80\xA6";
		while (Str.size() > 1 && Measurer.Width(Str + Ellipsis, Face, Style.Size) > Style.MaxWidth)
		{
			PopCodepoint(Str);
		}
		Str += Ellipsis;
	}
	const float Wd = Measurer.Width(Str, Face, Style.Size);
	const float X0 = X - (Style.HAlign == Align::Center ? Wd * 0.5f : Style.HAlign == Align::Right ? Wd : 0.0f);
	float Base = Y;
	switch (Style.VAlign)
	{
	case Baseline::Middle: Base = Y + 0.29f * Style.Size; break;
	case Baseline::Top: Base = Y + 0.79f * Style.Size; break;
	case Baseline::Bottom: Base = Y - 0.21f * Style.Size; break;
	default: break;
	}
	const float S0 = State.Xf.UniformScale();
	const Vec2 P = State.Xf.Apply({X0, Base});
	const float SizeDev = Style.Size * S0;
	const Color Col = Faded(Style.Col);
	if (Col.A > 0.003f && SizeDev > 0.5f)
	{
		TextItem Item;
		Item.Text = Str;
		Item.SizePx = SizeDev;
		Item.Face = Face;
		Item.Col = Col;
		Item.X = P.X;
		Item.BaselineY = P.Y;
		Item.Y = P.Y - Measurer.Ascent(Face, SizeDev);
		Out.AddText(Item);
	}
	return Wd;
}
} // namespace ui
} // namespace ss
