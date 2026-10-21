// Runs the golden vectors exported from the TypeScript build.
#include "ShortStack/Golden.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		std::fprintf(stderr, "usage: golden_test <golden_vectors.txt> [--rewrite <out.txt>]\n");
		return 2;
	}
	// --rewrite: after a deliberate change to the bots (made in web/src/core/ai/bot.ts too), the file with every
	// tournament's ticks and result as this build plays them; the run's report is still the check against the old file.
	const char* RewriteTo = argc >= 4 && std::string(argv[2]) == "--rewrite" ? argv[3] : nullptr;
	std::ifstream In(argv[1], std::ios::binary);
	if (!In)
	{
		std::fprintf(stderr, "cannot open %s\n", argv[1]);
		return 2;
	}
	std::stringstream Buf;
	Buf << In.rdbuf();
	const auto T0 = std::chrono::steady_clock::now();
	const ss::GoldenResult R = ss::RunGoldenVectors(Buf.str(), RewriteTo != nullptr);
	if (RewriteTo)
	{
		std::ofstream Out(RewriteTo, std::ios::binary);
		Out << R.Rewritten;
		std::printf("rewrote %s\n", RewriteTo);
	}
	const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
	std::printf("%s", R.Report.c_str());
	std::printf("golden vectors: %d passed, %d failed, %d skipped (%.0f ms)\n", R.Passed, R.Failed, R.Skipped, Ms);
	return R.Failed == 0 ? 0 : 1;
}
