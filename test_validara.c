#include "greatest.h"
#include "validara.h"

#include <stdio.h>
#include <string.h>

TEST sample_project_has_complete_offer_data(void) {
    ValidaraProject p;
    validara_load_sample(&p);
    ASSERT_STR_EQ("Website Evidence Audit Sprint", p.offer.name);
    ASSERT(p.offer.required_input_count >= 3);
    ASSERT(p.offer.exclusion_count >= 3);
    ASSERT(strlen(p.offer.intended_use) > 20);
    PASS();
}

TEST default_criteria_cover_validation_dimensions(void) {
    ValidaraCriterion c[VALIDARA_CRITERIA_COUNT];
    validara_default_criteria(c);
    ASSERT_EQ(VALIDARA_SPECIFICITY, c[0].kind);
    ASSERT_EQ(VALIDARA_ACCURACY, c[1].kind);
    ASSERT_EQ(VALIDARA_PRECISION, c[2].kind);
    ASSERT_EQ(VALIDARA_RANGE, c[3].kind);
    ASSERT_EQ(VALIDARA_ROBUSTNESS, c[4].kind);
    ASSERT_EQ(VALIDARA_SYSTEM_SUITABILITY, c[5].kind);
    ASSERT_EQ(VALIDARA_CHANGE_CONTROL, c[6].kind);
    PASS();
}

TEST draft_when_core_offer_fields_missing(void) {
    ValidaraProject p;
    validara_init(&p);
    ValidaraAssessment a = validara_assess(&p);
    ASSERT_EQ(VALIDARA_DRAFT, a.state);
    ASSERT(a.warning_count >= 4);
    PASS();
}

TEST no_launch_without_pilot_runs(void) {
    ValidaraProject p;
    validara_load_sample(&p);
    p.pilot_count = 0;
    ValidaraAssessment a = validara_assess(&p);
    ASSERT_EQ(VALIDARA_DRAFT, a.state);
    ASSERT_EQ(0, a.pilot_score);
    PASS();
}

TEST one_pilot_is_pilot_only_even_with_strong_scores(void) {
    ValidaraProject p;
    validara_load_sample(&p);
    p.pilot_count = 1;
    for (int i = 0; i < VALIDARA_CRITERIA_COUNT; i++) p.criteria[i].score = 95;
    p.pilots[0].duration_variance_pct = 0;
    p.pilots[0].scope_variance_pct = 0;
    p.pilots[0].missing_inputs = 0;
    p.pilots[0].deviation_severity = 0;
    p.pilots[0].fit_score = 100;
    ValidaraAssessment a = validara_assess(&p);
    ASSERT_EQ(VALIDARA_PILOT_ONLY, a.state);
    ASSERT(a.overall_score >= 90);
    PASS();
}

TEST sample_assesses_as_restricted_launch(void) {
    ValidaraProject p;
    validara_load_sample(&p);
    ValidaraAssessment a = validara_assess(&p);
    ASSERT_EQ(VALIDARA_RESTRICTED_LAUNCH, a.state);
    ASSERT(a.overall_score >= 70);
    ASSERT(a.warning_count > 0);
    PASS();
}

TEST excellent_evidence_can_reach_validated_launch(void) {
    ValidaraProject p;
    validara_load_sample(&p);
    for (int i = 0; i < VALIDARA_CRITERIA_COUNT; i++) p.criteria[i].score = 96;
    p.pilot_count = 0;
    ValidaraPilotRun r1 = {"good fit", "pass", 2, 1, 0, 0, 98};
    ValidaraPilotRun r2 = {"good fit", "pass", -1, 2, 0, 0, 97};
    ValidaraPilotRun r3 = {"good fit", "pass", 4, 0, 0, 0, 99};
    ASSERT(validara_add_pilot(&p, r1));
    ASSERT(validara_add_pilot(&p, r2));
    ASSERT(validara_add_pilot(&p, r3));
    ValidaraAssessment a = validara_assess(&p);
    ASSERT_EQ(VALIDARA_VALIDATED_LAUNCH, a.state);
    ASSERT_EQ(0, a.warning_count);
    PASS();
}

TEST poor_pilot_variance_reduces_readiness(void) {
    ValidaraProject p;
    validara_load_sample(&p);
    for (int i = 0; i < VALIDARA_CRITERIA_COUNT; i++) p.criteria[i].score = 90;
    p.pilot_count = 0;
    ValidaraPilotRun r1 = {"poor fit", "fail", 80, 70, 3, 3, 30};
    ValidaraPilotRun r2 = {"adjacent fit", "fail", 60, 50, 2, 2, 40};
    validara_add_pilot(&p, r1);
    validara_add_pilot(&p, r2);
    ValidaraAssessment a = validara_assess(&p);
    ASSERT_EQ(VALIDARA_PILOT_ONLY, a.state);
    ASSERT(a.pilot_score < 40);
    PASS();
}

TEST adding_required_inputs_and_exclusions_respects_capacity(void) {
    ValidaraProject p;
    validara_init(&p);
    for (int i = 0; i < VALIDARA_MAX_LIST; i++) {
        ASSERT(validara_add_required_input(&p.offer, "input"));
        ASSERT(validara_add_exclusion(&p.offer, "exclusion"));
    }
    ASSERT_FALSE(validara_add_required_input(&p.offer, "extra"));
    ASSERT_FALSE(validara_add_exclusion(&p.offer, "extra"));
    ASSERT_EQ(VALIDARA_MAX_LIST, p.offer.required_input_count);
    ASSERT_EQ(VALIDARA_MAX_LIST, p.offer.exclusion_count);
    PASS();
}

TEST markdown_export_contains_method_label_fields(void) {
    ValidaraProject p;
    validara_load_sample(&p);
    ValidaraAssessment a = validara_assess(&p);
    const char *path = "/tmp/validara-test-report.md";
    ASSERT(validara_export_markdown(path, &p, &a));
    FILE *f = fopen(path, "r");
    ASSERT(f != NULL);
    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    ASSERT(strstr(buf, "# Validara Method Label") != NULL);
    ASSERT(strstr(buf, "## Required Inputs") != NULL);
    ASSERT(strstr(buf, "## Exclusions") != NULL);
    ASSERT(strstr(buf, "## Change Control") != NULL);
    PASS();
}

SUITE(validara_suite) {
    RUN_TEST(sample_project_has_complete_offer_data);
    RUN_TEST(default_criteria_cover_validation_dimensions);
    RUN_TEST(draft_when_core_offer_fields_missing);
    RUN_TEST(no_launch_without_pilot_runs);
    RUN_TEST(one_pilot_is_pilot_only_even_with_strong_scores);
    RUN_TEST(sample_assesses_as_restricted_launch);
    RUN_TEST(excellent_evidence_can_reach_validated_launch);
    RUN_TEST(poor_pilot_variance_reduces_readiness);
    RUN_TEST(adding_required_inputs_and_exclusions_respects_capacity);
    RUN_TEST(markdown_export_contains_method_label_fields);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(validara_suite);
    GREATEST_MAIN_END();
}
