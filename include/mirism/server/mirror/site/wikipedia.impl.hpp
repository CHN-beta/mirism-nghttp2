# pragma once
# include <mirism/server/mirror/site/wikipedia.hpp>
# include <mirism/server/utils/string.hpp>

namespace mirism::server::mirror::site
{
	inline
	Wikipedia::Wikipedia()
	{
		Logger::Guard log(fmt::ptr(this));
		PatchMap_.insert
		({
			PatchTiming::AtResponseBodyPatch,
			[]
			(
				const std::unique_ptr<Request>& request,
				const std::unique_ptr<Response>& response,
				const DomainStrategy& domain_strategy
			) -> bool
			{
				Logger::Guard log(request, response);
				if (!request || !response || !response->HandledContent)
					return true;

				// /w/load.php?* javascript
				// patch "local":"/w/load.php" "metawiki":"//meta.wikimedia.org/w/load.php" ...
				// patch url(/xxxxx)
				// patch mw.loader.load('//xxx.xxx.xxx/xxx')
				if (request->Path.starts_with("/w/load.php?"))
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Javascript*>(response->HandledContent.get());
						content_ptr != nullptr && dynamic_cast<content::text::Html*>(content_ptr) == nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content,
								R"r("(local|metawiki|wgPopupsRestGatewayEndpoint|restbaseUrl|fullRestbaseUrl|baseUrl|serviceUri|checkLoggedInURL)"(\s*):(\s*)"([^"]+)")r"_re,
								[&](const std::smatch& match)
								{
									return R"("{}"{}:{}"{}")"_f
									(
										match[1].str(),
										match[2].str(),
										match[3].str(),
										url_patch(*request, domain_strategy, match[4].str())
									);
								}
							);
							content = utils::string::replace
							(
								content,
								R"r(\burl\((/[^'"\)]+)\))r"_re,
								[&](const std::smatch& match)
								{
									return "url({})"_f(url_patch(*request, domain_strategy, match[1].str()));
								}
							);
							content = utils::string::replace
							(
								content,
								R"r(\bmw\.loader\.load\('(//[^'"\)]+)'\))r"_re,
								[&](const std::smatch& match)
								{
									return "mw.loader.load('{}')"_f
									(
										url_patch(*request, domain_strategy, match[1].str())
									);
								}
							);
						});

				// /wiki/ html "wgInternalRedirectTargetUrl":"/wiki/Subsidence_(atmosphere)"
				if (request->Path.starts_with("/wiki/"))
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Html*>(response->HandledContent.get());
						content_ptr != nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content,
								R"r("wgInternalRedirectTargetUrl":"([^"]+)")r"_re,
								[&](const std::smatch& match)
								{
									return R"r("wgInternalRedirectTargetUrl":"{}")r"_f
									(url_patch(*request, domain_strategy, match[1].str()));
								}
							);
						});

				// /api/rest_v1/page/summary json patch https://
				if (request->Path.starts_with("/api/rest_v1/page/summary/"))
					if
					(
						auto* content_ptr = dynamic_cast<content::text::Json*>(response->HandledContent.get());
						content_ptr != nullptr
					)
						content_ptr->patch([&](content::Base::content_t& content)
						{
							content = utils::string::replace
							(
								content, R"r("https\://([^"]+)")r"_re,
								[&](const std::smatch& match)
								{
									return R"("{}")"_f
									(
										url_patch(*request, domain_strategy, "https://" + match[1].str())
									);
								}
							);
						});
				return true;
			}
		});
	}

	inline
	const DomainStrategy& Wikipedia::get_domain_strategy() const
	{
		static DomainStrategy domain_strategy
		(
		{
				{
					"wikipedia",
					"wikipedia.org",
					{
						"en.wikipedia.org",
						"ceb.wikipedia.org",
						"sv.wikipedia.org",
						"de.wikipedia.org",
						"fr.wikipedia.org",
						"nl.wikipedia.org",
						"ru.wikipedia.org",
						"it.wikipedia.org",
						"es.wikipedia.org",
						"pl.wikipedia.org",
						"arz.wikipedia.org",
						"ja.wikipedia.org",
						"vi.wikipedia.org",
						"war.wikipedia.org",
						"zh.wikipedia.org",
						"ar.wikipedia.org",
						"uk.wikipedia.org",
						"pt.wikipedia.org",
						"fa.wikipedia.org",
						"ca.wikipedia.org",
						"sr.wikipedia.org",
						"id.wikipedia.org",
						"no.wikipedia.org",
						"ko.wikipedia.org",
						"fi.wikipedia.org",
						"hu.wikipedia.org",
						"cs.wikipedia.org",
						"sh.wikipedia.org",
						"zh-min-nan.wikipedia.org",
						"ro.wikipedia.org",
						"tr.wikipedia.org",
						"eu.wikipedia.org",
						"ce.wikipedia.org",
						"ms.wikipedia.org",
						"tt.wikipedia.org",
						"eo.wikipedia.org",
						"he.wikipedia.org",
						"hy.wikipedia.org",
						"bg.wikipedia.org",
						"da.wikipedia.org",
						"azb.wikipedia.org",
						"sk.wikipedia.org",
						"kk.wikipedia.org",
						"min.wikipedia.org",
						"et.wikipedia.org",
						"hr.wikipedia.org",
						"be.wikipedia.org",
						"lt.wikipedia.org",
						"el.wikipedia.org",
						"simple.wikipedia.org",
						"az.wikipedia.org",
						"gl.wikipedia.org",
						"sl.wikipedia.org",
						"ur.wikipedia.org",
						"nn.wikipedia.org",
						"ka.wikipedia.org",
						"hi.wikipedia.org",
						"uz.wikipedia.org",
						"th.wikipedia.org",
						"ta.wikipedia.org",
						"la.wikipedia.org",
						"cy.wikipedia.org",
						"ast.wikipedia.org",
						"vo.wikipedia.org",
						"mk.wikipedia.org",
						"zh-yue.wikipedia.org",
						"bn.wikipedia.org",
						"lv.wikipedia.org",
						"tg.wikipedia.org",
						"my.wikipedia.org",
						"af.wikipedia.org",
						"mg.wikipedia.org",
						"oc.wikipedia.org",
						"bs.wikipedia.org",
						"sq.wikipedia.org",
						"nds.wikipedia.org",
						"ky.wikipedia.org",
						"mr.wikipedia.org",
						"be-tarask.wikipedia.org",
						"ml.wikipedia.org",
						"new.wikipedia.org",
						"te.wikipedia.org",
						"br.wikipedia.org",
						"vec.wikipedia.org",
						"pms.wikipedia.org",
						"sw.wikipedia.org",
						"jv.wikipedia.org",
						"pnb.wikipedia.org",
						"ht.wikipedia.org",
						"su.wikipedia.org",
						"lb.wikipedia.org",
						"ba.wikipedia.org",
						"ga.wikipedia.org",
						"szl.wikipedia.org",
						"is.wikipedia.org",
						"tl.wikipedia.org",
						"lmo.wikipedia.org",
						"cv.wikipedia.org",
						"fy.wikipedia.org",
						"wuu.wikipedia.org",
						"sco.wikipedia.org",
						"an.wikipedia.org",
						"diq.wikipedia.org",
						"ku.wikipedia.org",
						"pa.wikipedia.org",
						"yo.wikipedia.org",
						"ne.wikipedia.org",
						"ckb.wikipedia.org",
						"bar.wikipedia.org",
						"io.wikipedia.org",
						"gu.wikipedia.org",
						"als.wikipedia.org",
						"kn.wikipedia.org",
						"scn.wikipedia.org",
						"bpy.wikipedia.org",
						"ia.wikipedia.org",
						"qu.wikipedia.org",
						"mn.wikipedia.org",
						"si.wikipedia.org",
						"bat-smg.wikipedia.org",
						"nv.wikipedia.org",
						"xmf.wikipedia.org",
						"or.wikipedia.org",
						"cdo.wikipedia.org",
						"ilo.wikipedia.org",
						"gd.wikipedia.org",
						"yi.wikipedia.org",
						"am.wikipedia.org",
						"nap.wikipedia.org",
						"avk.wikipedia.org",
						"sd.wikipedia.org",
						"bug.wikipedia.org",
						"hsb.wikipedia.org",
						"mai.wikipedia.org",
						"fo.wikipedia.org",
						"os.wikipedia.org",
						"frr.wikipedia.org",
						"map-bms.wikipedia.org",
						"li.wikipedia.org",
						"mzn.wikipedia.org",
						"sah.wikipedia.org",
						"eml.wikipedia.org",
						"ps.wikipedia.org",
						"ace.wikipedia.org",
						"sa.wikipedia.org",
						"gor.wikipedia.org",
						"bcl.wikipedia.org",
						"wa.wikipedia.org",
						"zh-classical.wikipedia.org",
						"crh.wikipedia.org",
						"lij.wikipedia.org",
						"mrj.wikipedia.org",
						"mhr.wikipedia.org",
						"ha.wikipedia.org",
						"hif.wikipedia.org",
						"hak.wikipedia.org",
						"roa-tara.wikipedia.org",
						"shn.wikipedia.org",
						"pam.wikipedia.org",
						"hyw.wikipedia.org",
						"as.wikipedia.org",
						"zu.wikipedia.org",
						"nso.wikipedia.org",
						"km.wikipedia.org",
						"rue.wikipedia.org",
						"ie.wikipedia.org",
						"ban.wikipedia.org",
						"se.wikipedia.org",
						"bh.wikipedia.org",
						"vls.wikipedia.org",
						"nds-nl.wikipedia.org",
						"mi.wikipedia.org",
						"sn.wikipedia.org",
						"sc.wikipedia.org",
						"nah.wikipedia.org",
						"vep.wikipedia.org",
						"gan.wikipedia.org",
						"myv.wikipedia.org",
						"glk.wikipedia.org",
						"kab.wikipedia.org",
						"sat.wikipedia.org",
						"so.wikipedia.org",
						"ab.wikipedia.org",
						"bo.wikipedia.org",
						"tk.wikipedia.org",
						"co.wikipedia.org",
						"fiu-vro.wikipedia.org",
						"kv.wikipedia.org",
						"csb.wikipedia.org",
						"pcd.wikipedia.org",
						"frp.wikipedia.org",
						"udm.wikipedia.org",
						"ug.wikipedia.org",
						"gv.wikipedia.org",
						"ay.wikipedia.org",
						"zea.wikipedia.org",
						"kw.wikipedia.org",
						"nrm.wikipedia.org",
						"lez.wikipedia.org",
						"lfn.wikipedia.org",
						"gn.wikipedia.org",
						"stq.wikipedia.org",
						"mt.wikipedia.org",
						"mni.wikipedia.org",
						"ary.wikipedia.org",
						"mwl.wikipedia.org",
						"lo.wikipedia.org",
						"rm.wikipedia.org",
						"skr.wikipedia.org",
						"bjn.wikipedia.org",
						"olo.wikipedia.org",
						"lad.wikipedia.org",
						"gom.wikipedia.org",
						"koi.wikipedia.org",
						"fur.wikipedia.org",
						"ang.wikipedia.org",
						"dsb.wikipedia.org",
						"dty.wikipedia.org",
						"ext.wikipedia.org",
						"ln.wikipedia.org",
						"tyv.wikipedia.org",
						"cbk-zam.wikipedia.org",
						"smn.wikipedia.org",
						"dv.wikipedia.org",
						"ksh.wikipedia.org",
						"gag.wikipedia.org",
						"pfl.wikipedia.org",
						"pi.wikipedia.org",
						"pag.wikipedia.org",
						"av.wikipedia.org",
						"bxr.wikipedia.org",
						"awa.wikipedia.org",
						"tay.wikipedia.org",
						"haw.wikipedia.org",
						"pap.wikipedia.org",
						"ig.wikipedia.org",
						"xal.wikipedia.org",
						"rw.wikipedia.org",
						"krc.wikipedia.org",
						"za.wikipedia.org",
						"szy.wikipedia.org",
						"pdc.wikipedia.org",
						"kaa.wikipedia.org",
						"inh.wikipedia.org",
						"arc.wikipedia.org",
						"to.wikipedia.org",
						"nov.wikipedia.org",
						"kbp.wikipedia.org",
						"jam.wikipedia.org",
						"wo.wikipedia.org",
						"tpi.wikipedia.org",
						"na.wikipedia.org",
						"kbd.wikipedia.org",
						"atj.wikipedia.org",
						"ki.wikipedia.org",
						"tet.wikipedia.org",
						"tcy.wikipedia.org",
						"ak.wikipedia.org",
						"lld.wikipedia.org",
						"lg.wikipedia.org",
						"jbo.wikipedia.org",
						"bi.wikipedia.org",
						"roa-rup.wikipedia.org",
						"lbe.wikipedia.org",
						"kg.wikipedia.org",
						"ty.wikipedia.org",
						"mdf.wikipedia.org",
						"xh.wikipedia.org",
						"fj.wikipedia.org",
						"srn.wikipedia.org",
						"om.wikipedia.org",
						"gcr.wikipedia.org",
						"trv.wikipedia.org",
						"ltg.wikipedia.org",
						"sm.wikipedia.org",
						"chr.wikipedia.org",
						"nia.wikipedia.org",
						"nqo.wikipedia.org",
						"mnw.wikipedia.org",
						"pih.wikipedia.org",
						"got.wikipedia.org",
						"st.wikipedia.org",
						"kl.wikipedia.org",
						"mad.wikipedia.org",
						"tw.wikipedia.org",
						"cu.wikipedia.org",
						"ny.wikipedia.org",
						"tn.wikipedia.org",
						"ts.wikipedia.org",
						"bm.wikipedia.org",
						"rmy.wikipedia.org",
						"ve.wikipedia.org",
						"chy.wikipedia.org",
						"rn.wikipedia.org",
						"tum.wikipedia.org",
						"iu.wikipedia.org",
						"ss.wikipedia.org",
						"ks.wikipedia.org",
						"ch.wikipedia.org",
						"alt.wikipedia.org",
						"pnt.wikipedia.org",
						"ady.wikipedia.org",
						"ee.wikipedia.org",
						"ff.wikipedia.org",
						"ik.wikipedia.org",
						"din.wikipedia.org",
						"sg.wikipedia.org",
						"dz.wikipedia.org",
						"ti.wikipedia.org",
						"cr.wikipedia.org",
						"ng.wikipedia.org",
						"cho.wikipedia.org",
						"kj.wikipedia.org",
						"mh.wikipedia.org",
						"ho.wikipedia.org",
						"ii.wikipedia.org",
						"aa.wikipedia.org",
						"lrc.wikipedia.org",
						"mus.wikipedia.org",
						"hz.wikipedia.org",
						"kr.wikipedia.org"
					}
				},
				{
					"wikimedia",
					"wikimedia.org",
					{
						"wikimedia.org",
						"intake-analytics.wikimedia.org",
						"intake-logging.wikimedia.org",
						"login.wikimedia.org",
						"meta.wikimedia.org",
						"upload.wikimedia.org"
					}
				}
			},
			{},
			{}
		);
		return domain_strategy;
	}
}
