# pragma once
# include <mirism/server/mirror/site/scihub/se.hpp>

namespace mirism::server::mirror::site::scihub
{
	inline
	const DomainStrategy& Se::get_domain_strategy() const
	{
		static DomainStrategy domain_strategy
		(
			{
				{
					"scihub.se",
					"sci-hub.se",
					{
						"sci-hub.se",
						"www-tandfonline-com.sci-hub.se",
						"deu.sci-hub.se",
						"ma.sci-hub.se",
						"in.sci-hub.se",
						"gov.sci-hub.se",
						"it.sci-hub.se",
						"google.sci-hub.se",
						"be.sci-hub.se",
						"cottone.sci-hub.se",
						"ww25.sci-hub.se",
						"125.sci-hub.se",
						"zero.sci-hub.se",
						"amazon.sci-hub.se",
						"wwws.sci-hub.se",
						"dx.sci-hub.se",
						"mx.sci-hub.se",
						"c.sci-hub.se",
						"app.sci-hub.se",
						"moscow.sci-hub.se",
						"www.sci-hub.se",
						"donate2.sci-hub.se",
						"xs.sci-hub.se",
						"ar-iiarjournals-org.sci-hub.se",
						"learningstg.sci-hub.se",
						"8.sci-hub.se",
						"gradschool.sci-hub.se",
						"cu.sci-hub.se",
						"hectoryoeuk.sci-hub.se",
						"au.sci-hub.se",
						"fortelugu.sci-hub.se",
						"fr.sci-hub.se",
						"link-springer-com.sci-hub.se",
						"cy.sci-hub.se",
						"99lb.sci-hub.se",
						"wwe.sci-hub.se",
						"hub.sci-hub.se",
						"group.sci-hub.se",
						"info.sci-hub.se",
						"is.sci-hub.se",
						"hyperstatic.sci-hub.se",
						"www5.sci-hub.se",
						"jasminegolden122.sci-hub.se",
						"floodingrepair84940.sci-hub.se",
						"2.sci-hub.se",
						"ssrq04.sci-hub.se",
						"4236.sci-hub.se",
						"gfsoso.sci-hub.se",
						"ww1.sci-hub.se",
						"st.sci-hub.se",
						"backup.sci-hub.se",
						"de.sci-hub.se",
						"analysis52851.sci-hub.se",
						"dentistloan51594.sci-hub.se",
						"isa.sci-hub.se",
						"libjjen.sci-hub.se",
						"libgen.sci-hub.se",
						"wwww.sci-hub.se",
						"matrix.sci-hub.se",
						"board.sci-hub.se",
						"do.sci-hub.se",
						"pubs-rsc-org.sci-hub.se",
						"twin.sci-hub.se",
						"orghttps.sci-hub.se",
						"ww38.sci-hub.se",
						"httpswww.sci-hub.se",
						"wwwcms1.sci-hub.se",
						"wp1.sci-hub.se",
						"e502.sci-hub.se",
						"doh.sci-hub.se",
						"pub.sci-hub.se",
						"www-intmedpress-com.sci-hub.se",
						"co.sci-hub.se",
						"dabamirror.sci-hub.se",
						"cybe.sci-hub.se",
						"ieee.sci-hub.se",
						"4.sci-hub.se",
						"cust3236.sci-hub.se",
						"ca.sci-hub.se",
						"kor.sci-hub.se",
						"unblocked.sci-hub.se",
						"eurekaselect.sci-hub.se",
						"files.sci-hub.se",
						"coms.sci-hub.se",
						"cn.sci-hub.se",
						"cyb.sci-hub.se",
						"www25.sci-hub.se",
						"navigation-plugins-api.sci-hub.se",
						"annualreviews.sci-hub.se",
						"codydmsxc.sci-hub.se",
						"c0269.sci-hub.se",
						"art.sci-hub.se",
						"swww.sci-hub.se",
						"id.sci-hub.se",
						"top.sci-hub.se",
						"ww12.sci-hub.se",
						"cms.sci-hub.se",
						"eduphp.sci-hub.se",
						"babaft.sci-hub.se",
						"emedien.sci-hub.se",
						"gitlab.sci-hub.se"
					}
				},
			},
			{R"(.*\.sci\-hub\.se)"_re},
			{}
		);
		return domain_strategy;
	}
}