#ifndef VALIDARA_H
#define VALIDARA_H

#include <stddef.h>
#include <stdio.h>

#define VALIDARA_MAX_TEXT 128
#define VALIDARA_MAX_LIST 8
#define VALIDARA_MAX_PILOTS 16
#define VALIDARA_CRITERIA_COUNT 7

typedef enum {
    VALIDARA_DRAFT = 0,
    VALIDARA_PILOT_ONLY,
    VALIDARA_RESTRICTED_LAUNCH,
    VALIDARA_VALIDATED_LAUNCH
} ValidaraReadinessState;

typedef enum {
    VALIDARA_SPECIFICITY = 0,
    VALIDARA_ACCURACY,
    VALIDARA_PRECISION,
    VALIDARA_RANGE,
    VALIDARA_ROBUSTNESS,
    VALIDARA_SYSTEM_SUITABILITY,
    VALIDARA_CHANGE_CONTROL
} ValidaraCriterionKind;

typedef struct {
    char name[VALIDARA_MAX_TEXT];
    char target_client[VALIDARA_MAX_TEXT];
    char intended_use[VALIDARA_MAX_TEXT];
    char required_inputs[VALIDARA_MAX_LIST][VALIDARA_MAX_TEXT];
    int required_input_count;
    char exclusions[VALIDARA_MAX_LIST][VALIDARA_MAX_TEXT];
    int exclusion_count;
    int target_duration_minutes;
    char promised_outputs[VALIDARA_MAX_TEXT];
    char revision_boundary[VALIDARA_MAX_TEXT];
} ValidaraOffer;

typedef struct {
    ValidaraCriterionKind kind;
    char label[64];
    char measurement[VALIDARA_MAX_TEXT];
    int threshold;
    int score;
    int required;
} ValidaraCriterion;

typedef struct {
    char client_fit[64];
    char outcome[64];
    int duration_variance_pct;
    int scope_variance_pct;
    int missing_inputs;
    int deviation_severity; /* 0 none, 1 minor, 2 moderate, 3 major */
    int fit_score;          /* 0-100 */
} ValidaraPilotRun;

typedef struct {
    ValidaraOffer offer;
    ValidaraCriterion criteria[VALIDARA_CRITERIA_COUNT];
    ValidaraPilotRun pilots[VALIDARA_MAX_PILOTS];
    int pilot_count;
    char change_note[VALIDARA_MAX_TEXT];
} ValidaraProject;

typedef struct {
    ValidaraReadinessState state;
    int protocol_score;
    int pilot_score;
    int overall_score;
    int warning_count;
    char warnings[12][160];
} ValidaraAssessment;

void validara_init(ValidaraProject *project);
void validara_load_sample(ValidaraProject *project);
int validara_add_required_input(ValidaraOffer *offer, const char *input);
int validara_add_exclusion(ValidaraOffer *offer, const char *exclusion);
int validara_add_pilot(ValidaraProject *project, ValidaraPilotRun run);
void validara_default_criteria(ValidaraCriterion criteria[VALIDARA_CRITERIA_COUNT]);
ValidaraAssessment validara_assess(const ValidaraProject *project);
const char *validara_state_name(ValidaraReadinessState state);
const char *validara_criterion_name(ValidaraCriterionKind kind);
void validara_print_dashboard(FILE *out, const ValidaraProject *project, const ValidaraAssessment *assessment);
int validara_export_markdown(const char *path, const ValidaraProject *project, const ValidaraAssessment *assessment);
void validara_print_report(FILE *out, const ValidaraProject *project, const ValidaraAssessment *assessment);
void validara_print_interactive(FILE *out, const ValidaraProject *project, const ValidaraAssessment *assessment);

#endif
