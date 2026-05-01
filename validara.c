#include "validara.h"

#include <stdlib.h>
#include <string.h>

static void copy_text(char *dst, size_t n, const char *src) {
    if (!dst || n == 0) return;
    if (!src) src = "";
    snprintf(dst, n, "%s", src);
}

static int clamp_int(int v, int low, int high) {
    if (v < low) return low;
    if (v > high) return high;
    return v;
}

static int nonempty(const char *s) {
    return s && s[0] != '\0';
}

static void add_warning(ValidaraAssessment *a, const char *message) {
    if (a->warning_count < 12) {
        copy_text(a->warnings[a->warning_count], sizeof(a->warnings[a->warning_count]), message);
    }
    a->warning_count++;
}

void validara_default_criteria(ValidaraCriterion criteria[VALIDARA_CRITERIA_COUNT]) {
    const char *labels[VALIDARA_CRITERIA_COUNT] = {
        "Specificity / selectivity",
        "Accuracy",
        "Precision / repeatability",
        "Range",
        "Robustness",
        "System suitability",
        "Change control"
    };
    const char *methods[VALIDARA_CRITERIA_COUNT] = {
        "Intake screens good-fit clients from adjacent requests",
        "Outputs match the real client need and evidence",
        "Similar clients receive similar time, scope, and quality",
        "Valid client complexity, input quality, budget, and urgency are bounded",
        "Offer tolerates missing files, late replies, and context changes",
        "Pre-flight checks confirm access, materials, authority, timeline, and budget",
        "Promise, price, exclusion, intake, and handoff edits are versioned"
    };
    for (int i = 0; i < VALIDARA_CRITERIA_COUNT; i++) {
        criteria[i].kind = (ValidaraCriterionKind)i;
        copy_text(criteria[i].label, sizeof(criteria[i].label), labels[i]);
        copy_text(criteria[i].measurement, sizeof(criteria[i].measurement), methods[i]);
        criteria[i].threshold = 70;
        criteria[i].score = 0;
        criteria[i].required = 1;
    }
}

void validara_init(ValidaraProject *project) {
    if (!project) return;
    memset(project, 0, sizeof(*project));
    project->offer.target_duration_minutes = 120;
    validara_default_criteria(project->criteria);
}

int validara_add_required_input(ValidaraOffer *offer, const char *input) {
    if (!offer || !nonempty(input) || offer->required_input_count >= VALIDARA_MAX_LIST) return 0;
    copy_text(offer->required_inputs[offer->required_input_count++], VALIDARA_MAX_TEXT, input);
    return 1;
}

int validara_add_exclusion(ValidaraOffer *offer, const char *exclusion) {
    if (!offer || !nonempty(exclusion) || offer->exclusion_count >= VALIDARA_MAX_LIST) return 0;
    copy_text(offer->exclusions[offer->exclusion_count++], VALIDARA_MAX_TEXT, exclusion);
    return 1;
}

int validara_add_pilot(ValidaraProject *project, ValidaraPilotRun run) {
    if (!project || project->pilot_count >= VALIDARA_MAX_PILOTS) return 0;
    run.duration_variance_pct = clamp_int(run.duration_variance_pct, -100, 300);
    run.scope_variance_pct = clamp_int(run.scope_variance_pct, -100, 300);
    run.missing_inputs = clamp_int(run.missing_inputs, 0, 99);
    run.deviation_severity = clamp_int(run.deviation_severity, 0, 3);
    run.fit_score = clamp_int(run.fit_score, 0, 100);
    project->pilots[project->pilot_count++] = run;
    return 1;
}

void validara_load_sample(ValidaraProject *project) {
    validara_init(project);
    copy_text(project->offer.name, sizeof(project->offer.name), "Website Evidence Audit Sprint");
    copy_text(project->offer.target_client, sizeof(project->offer.target_client), "Solo service business with an existing website and one primary offer");
    copy_text(project->offer.intended_use, sizeof(project->offer.intended_use), "Diagnose conversion blockers and produce a prioritized 10-point repair plan");
    copy_text(project->offer.promised_outputs, sizeof(project->offer.promised_outputs), "Annotated findings, priority matrix, 45-minute review call, and action checklist");
    copy_text(project->offer.revision_boundary, sizeof(project->offer.revision_boundary), "One clarification pass; redesign, copywriting, and implementation are excluded");
    project->offer.target_duration_minutes = 120;
    validara_add_required_input(&project->offer, "Public website URL");
    validara_add_required_input(&project->offer, "Analytics or inquiry screenshots");
    validara_add_required_input(&project->offer, "Primary offer and desired visitor action");
    validara_add_exclusion(&project->offer, "Pre-launch businesses without a live site");
    validara_add_exclusion(&project->offer, "Multi-location retail or ecommerce catalog audits");
    validara_add_exclusion(&project->offer, "Naming, logo, or full brand redesign projects");
    int scores[VALIDARA_CRITERIA_COUNT] = {88, 84, 79, 72, 74, 86, 76};
    for (int i = 0; i < VALIDARA_CRITERIA_COUNT; i++) project->criteria[i].score = scores[i];
    copy_text(project->change_note, sizeof(project->change_note), "Version 0.3 added logo-redesign exclusion after pilot 2 scope deviation.");

    ValidaraPilotRun a = {"good fit", "pass", 8, 5, 0, 0, 92};
    ValidaraPilotRun b = {"adjacent fit", "conditional pass", 22, 18, 1, 1, 78};
    ValidaraPilotRun c = {"good fit", "pass", -6, 4, 0, 0, 90};
    validara_add_pilot(project, a);
    validara_add_pilot(project, b);
    validara_add_pilot(project, c);
}

static int compute_protocol_score(const ValidaraProject *p, ValidaraAssessment *a) {
    int sum = 0;
    for (int i = 0; i < VALIDARA_CRITERIA_COUNT; i++) {
        int s = clamp_int(p->criteria[i].score, 0, 100);
        sum += s;
        if (p->criteria[i].required && s < p->criteria[i].threshold) {
            char w[160];
            snprintf(w, sizeof(w), "%s below threshold (%d < %d).", p->criteria[i].label, s, p->criteria[i].threshold);
            add_warning(a, w);
        }
    }
    return sum / VALIDARA_CRITERIA_COUNT;
}

static int compute_pilot_score(const ValidaraProject *p, ValidaraAssessment *a) {
    if (p->pilot_count == 0) {
        add_warning(a, "No pilot or dry-run records exist; launch readiness cannot be validated.");
        return 0;
    }
    int sum = 0;
    int severe = 0;
    int missing = 0;
    for (int i = 0; i < p->pilot_count; i++) {
        const ValidaraPilotRun *r = &p->pilots[i];
        int penalty = abs(r->duration_variance_pct) / 2 + abs(r->scope_variance_pct) / 2 + r->missing_inputs * 10 + r->deviation_severity * 15 + (100 - r->fit_score) / 3;
        int score = clamp_int(100 - penalty, 0, 100);
        sum += score;
        if (r->deviation_severity >= 2) severe++;
        if (r->missing_inputs > 0) missing++;
    }
    if (p->pilot_count < 2) add_warning(a, "Only one pilot recorded; evidence range remains narrow.");
    if (severe > 0) add_warning(a, "Moderate or major pilot deviations require corrective action before broad launch.");
    if (missing > 0) add_warning(a, "At least one pilot lacked required inputs; strengthen system suitability checks.");
    return sum / p->pilot_count;
}

ValidaraAssessment validara_assess(const ValidaraProject *project) {
    ValidaraAssessment a;
    memset(&a, 0, sizeof(a));
    if (!project) {
        a.state = VALIDARA_DRAFT;
        add_warning(&a, "No project loaded.");
        return a;
    }
    if (!nonempty(project->offer.name)) add_warning(&a, "Offer name is missing.");
    if (!nonempty(project->offer.target_client)) add_warning(&a, "Target client is missing.");
    if (!nonempty(project->offer.intended_use)) add_warning(&a, "Intended use is missing.");
    if (project->offer.required_input_count == 0) add_warning(&a, "Required inputs are not defined.");
    if (project->offer.exclusion_count == 0) add_warning(&a, "Exclusions are not defined.");
    if (!nonempty(project->offer.revision_boundary)) add_warning(&a, "Revision boundary is missing.");

    a.protocol_score = compute_protocol_score(project, &a);
    a.pilot_score = compute_pilot_score(project, &a);
    a.overall_score = (a.protocol_score * 55 + a.pilot_score * 45) / 100;

    int missing_core = !nonempty(project->offer.name) || !nonempty(project->offer.target_client) ||
                       !nonempty(project->offer.intended_use) || project->offer.required_input_count == 0 ||
                       project->offer.exclusion_count == 0;
    if (missing_core || project->pilot_count == 0 || a.protocol_score < 50) {
        a.state = VALIDARA_DRAFT;
    } else if (a.overall_score < 65 || project->pilot_count < 2) {
        a.state = VALIDARA_PILOT_ONLY;
    } else if (a.overall_score < 85 || a.warning_count > 2) {
        a.state = VALIDARA_RESTRICTED_LAUNCH;
    } else {
        a.state = VALIDARA_VALIDATED_LAUNCH;
    }
    return a;
}

const char *validara_state_name(ValidaraReadinessState state) {
    switch (state) {
        case VALIDARA_DRAFT: return "Draft";
        case VALIDARA_PILOT_ONLY: return "Pilot Only";
        case VALIDARA_RESTRICTED_LAUNCH: return "Restricted Launch";
        case VALIDARA_VALIDATED_LAUNCH: return "Validated Launch";
        default: return "Unknown";
    }
}

const char *validara_criterion_name(ValidaraCriterionKind kind) {
    switch (kind) {
        case VALIDARA_SPECIFICITY: return "Specificity";
        case VALIDARA_ACCURACY: return "Accuracy";
        case VALIDARA_PRECISION: return "Precision / repeatability";
        case VALIDARA_RANGE: return "Range";
        case VALIDARA_ROBUSTNESS: return "Robustness";
        case VALIDARA_SYSTEM_SUITABILITY: return "System suitability";
        case VALIDARA_CHANGE_CONTROL: return "Change control";
        default: return "Unknown";
    }
}

static void print_bar(FILE *out, int value) {
    int filled = clamp_int(value, 0, 100) / 5;
    fputc('[', out);
    for (int i = 0; i < 20; i++) fputc(i < filled ? '#' : '.', out);
    fprintf(out, "] %3d", clamp_int(value, 0, 100));
}

void validara_print_dashboard(FILE *out, const ValidaraProject *p, const ValidaraAssessment *a) {
    if (!out || !p || !a) return;
    fprintf(out, "+------------------------------------------------------------------+\n");
    fprintf(out, "| VALIDARA SERVICE METHOD READINESS                                |\n");
    fprintf(out, "+------------------------------------------------------------------+\n");
    fprintf(out, "Offer : %s\n", p->offer.name);
    fprintf(out, "Client: %s\n", p->offer.target_client);
    fprintf(out, "State : %s\n", validara_state_name(a->state));
    fprintf(out, "Overall  "); print_bar(out, a->overall_score); fprintf(out, "\n");
    fprintf(out, "Protocol "); print_bar(out, a->protocol_score); fprintf(out, "\n");
    fprintf(out, "Pilots   "); print_bar(out, a->pilot_score); fprintf(out, " (%d runs)\n", p->pilot_count);
    fprintf(out, "\nValidation criteria\n");
    for (int i = 0; i < VALIDARA_CRITERIA_COUNT; i++) {
        fprintf(out, "  %-25s ", validara_criterion_name(p->criteria[i].kind));
        print_bar(out, p->criteria[i].score);
        fprintf(out, "  threshold %d\n", p->criteria[i].threshold);
    }
    fprintf(out, "\nWarnings\n");
    if (a->warning_count == 0) fprintf(out, "  none\n");
    int shown = a->warning_count < 12 ? a->warning_count : 12;
    for (int i = 0; i < shown; i++) fprintf(out, "  ! %s\n", a->warnings[i]);
    fprintf(out, "+------------------------------------------------------------------+\n");
}

void validara_print_report(FILE *out, const ValidaraProject *p, const ValidaraAssessment *a) {
    if (!out || !p || !a) return;
    fprintf(out, "# Validara Method Label: %s\n\n", p->offer.name);
    fprintf(out, "**Readiness:** %s  \n", validara_state_name(a->state));
    fprintf(out, "**Overall score:** %d/100  \n", a->overall_score);
    fprintf(out, "**Protocol score:** %d/100  \n", a->protocol_score);
    fprintf(out, "**Pilot score:** %d/100  \n\n", a->pilot_score);
    fprintf(out, "## Intended Use\n%s\n\n", p->offer.intended_use);
    fprintf(out, "## Target Client\n%s\n\n", p->offer.target_client);
    fprintf(out, "## Required Inputs\n");
    for (int i = 0; i < p->offer.required_input_count; i++) fprintf(out, "- %s\n", p->offer.required_inputs[i]);
    fprintf(out, "\n## Exclusions\n");
    for (int i = 0; i < p->offer.exclusion_count; i++) fprintf(out, "- %s\n", p->offer.exclusions[i]);
    fprintf(out, "\n## Promised Outputs\n%s\n\n", p->offer.promised_outputs);
    fprintf(out, "## Timeline and Revision Boundary\nTarget duration: %d minutes.  \n%s\n\n", p->offer.target_duration_minutes, p->offer.revision_boundary);
    fprintf(out, "## Validation Criteria\n");
    for (int i = 0; i < VALIDARA_CRITERIA_COUNT; i++) {
        fprintf(out, "- **%s:** %d/%d — %s\n", p->criteria[i].label, p->criteria[i].score, p->criteria[i].threshold, p->criteria[i].measurement);
    }
    fprintf(out, "\n## Pilot Runs\n");
    for (int i = 0; i < p->pilot_count; i++) {
        const ValidaraPilotRun *r = &p->pilots[i];
        fprintf(out, "- %s: %s; duration variance %+d%%; scope variance %+d%%; missing inputs %d; deviation severity %d; client fit %d/100.\n",
                r->client_fit, r->outcome, r->duration_variance_pct, r->scope_variance_pct, r->missing_inputs, r->deviation_severity, r->fit_score);
    }
    fprintf(out, "\n## Warnings and Controls\n");
    if (a->warning_count == 0) fprintf(out, "- No active warnings. Maintain change-control review after each sale.\n");
    int shown = a->warning_count < 12 ? a->warning_count : 12;
    for (int i = 0; i < shown; i++) fprintf(out, "- %s\n", a->warnings[i]);
    fprintf(out, "\n## Change Control\n%s\n", nonempty(p->change_note) ? p->change_note : "No change-control entry recorded yet.");
}

int validara_export_markdown(const char *path, const ValidaraProject *p, const ValidaraAssessment *a) {
    if (!path || !p || !a) return 0;
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    validara_print_report(f, p, a);
    int ok = fclose(f) == 0;
    return ok;
}

void validara_print_interactive(FILE *out, const ValidaraProject *p, const ValidaraAssessment *a) {
    if (!out || !p || !a) return;
    fprintf(out, "Validara TUI\n");
    fprintf(out, "1) Dashboard\n2) Method label preview\n3) Pilot summary\n4) Exit\n\n");
    validara_print_dashboard(out, p, a);
    fprintf(out, "\nTip: run `./validara --demo` for sample data or `./validara --export report.md` to write the method label.\n");
}
