/* SPDX-License-Identifier: GPL-2.0 */

#include <kunit/test.h>
#include <linux/delay.h>

struct rv_kunit_ctx {
	int reactions, expected;
	int cpu;
	struct task_struct *curr;
};

#define RV_KUNIT_EXPECT_REACTION(test, ctx)                             \
	do {                                                            \
		KUNIT_EXPECT_EQ(test, ctx->reactions, ++ctx->expected); \
		if (ctx->reactions != ctx->expected)                    \
			ctx->expected = ctx->reactions;                 \
	} while (0)

#define RV_KUNIT_EXPECT_NO_REACTION(test, ctx)                        \
	do {                                                          \
		KUNIT_EXPECT_EQ(test, ctx->reactions, ctx->expected); \
		if (ctx->reactions != ctx->expected)                  \
			ctx->expected = ctx->reactions;               \
	} while (0)

#ifdef CONFIG_RV_MON_SCO
extern void rv_test_sco(struct kunit *test);
#else
static inline void rv_test_sco(struct kunit *test)
{
	kunit_skip(test, "Monitor not enabled\n");
}
#endif

#ifdef CONFIG_RV_MON_SSSW
extern void rv_test_sssw(struct kunit *test);
#else
static inline void rv_test_sssw(struct kunit *test)
{
	kunit_skip(test, "Monitor not enabled\n");
}
#endif

#ifdef CONFIG_RV_MON_STS
extern void rv_test_sts(struct kunit *test);
#else
static inline void rv_test_sts(struct kunit *test)
{
	kunit_skip(test, "Monitor not enabled\n");
}
#endif

#ifdef CONFIG_RV_MON_OPID
extern void rv_test_opid(struct kunit *test);
#else
static inline void rv_test_opid(struct kunit *test)
{
	kunit_skip(test, "Monitor not enabled\n");
}
#endif

#ifdef CONFIG_RV_MON_NOMISS
extern void rv_test_nomiss(struct kunit *test);
#else
static inline void rv_test_nomiss(struct kunit *test)
{
	kunit_skip(test, "Monitor not enabled\n");
}
#endif
