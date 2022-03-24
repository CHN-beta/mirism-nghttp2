# pragma once

# include <mirism/common.impl.hpp>
# include <mirism/logger.impl.hpp>
# include <mirism/server.impl.hpp>

# include <mirism/server/common.impl.hpp>
# include <mirism/server/base.impl.hpp>
# include <mirism/server/synchronized.impl.hpp>
# include <mirism/server/mirror.impl.hpp>
# include <mirism/server/api.impl.hpp>

# include <mirism/server/utils/string.impl.hpp>
# include <mirism/server/utils/atomic.impl.hpp>
# include <mirism/server/utils/pipe.impl.hpp>

# include <mirism/server/mirror/common.impl.hpp>
# include <mirism/server/mirror/patch.impl.hpp>

# include <mirism/server/mirror/content/base.impl.hpp>
# include <mirism/server/mirror/content/binary.impl.hpp>
# include <mirism/server/mirror/content/text/base.impl.hpp>
# include <mirism/server/mirror/content/text/css.impl.hpp>
# include <mirism/server/mirror/content/text/javascript.impl.hpp>
# include <mirism/server/mirror/content/text/html.impl.hpp>
# include <mirism/server/mirror/content/text/json.impl.hpp>
# include <mirism/server/mirror/content/text/plain.impl.hpp>

# include <mirism/server/mirror/site/base.impl.hpp>
# include <mirism/server/mirror/site/wikipedia.impl.hpp>
# include <mirism/server/mirror/site/github.impl.hpp>
# include <mirism/server/mirror/site/scihub/base.impl.hpp>
# include <mirism/server/mirror/site/scihub/ru.impl.hpp>
# include <mirism/server/mirror/site/scihub/se.impl.hpp>
# include <mirism/server/mirror/site/hcaptcha/server.impl.hpp>
# include <mirism/server/mirror/site/hcaptcha/client.impl.hpp>
# include <mirism/server/mirror/site/google/scholar.impl.hpp>
# include <mirism/server/mirror/site/google/search.impl.hpp>
# include <mirism/server/mirror/site/google/youtube.impl.hpp>
# include <mirism/server/mirror/site/google/patents.impl.hpp>

# include <mirism/server/api/common.impl.hpp>
# include <mirism/server/api/compile.impl.hpp>

// TODO:
// api 增加日志
// api virtual const
// Parameter -> Query
// Content -> Body
// html 仅仅在需要的地方调用 Javascript 和 Css 的修改
// 增加 deflated 压缩的解压
// content::Base 的职能移入 Default
// mirism::server::mirror::content::text::patch 接受 string_view
// 所有的 Pipe 都应该在 shutdown_handler 中挂一个 hook
// dynamic cast 到 javascript 后，不再需要检查是否可以 dynamic cast 到 html
// atomic write 可以返回值
// 调整缩进及换行
// ShutdownCallbackHandler 按照引用传入
// pipe 的一些用于关闭的回调函数应该使用 weak_ptr
// shutdown_handler disable copy
// fetch 需要检查 header key 是否为小写
// if 增加 unlikely
// 增加 api 查看编译时间和 commit id
// 分离 path 和 query
// pipe 增加 write_async
// Logger 增加 new_thread
// 整理 Logger 的线程和对象监视接口
// 合并 group 列表时，检查重复的 host
// 去掉正则表达式中多余的 ^ 和 $
// mirror site 增加 requires 和 external_patch
// 对 unique_ptr 的 dynamic_pointer_cast
