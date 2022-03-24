# pragma once
# include <mirism/server/mirror/common.hpp>

namespace mirism::server::mirror::patch
{
	namespace detail_
	{
		struct PatchMapProxy
		{
			std::unordered_multimap<PatchTiming, patch_t> PatchMap;

			operator std::unordered_multimap<PatchTiming, patch_t>() const&;
			operator std::unordered_multimap<PatchTiming, patch_t>() &&;
			PatchMapProxy operator+(const PatchMapProxy&) const&;
			PatchMapProxy operator+(PatchMapProxy&&) &&;
		};
	}

	detail_::PatchMapProxy origin_restrict
	(std::unordered_set<std::string>, std::unordered_set<std::string>, std::string);

	detail_::PatchMapProxy check_leakage(const std::string&);
}