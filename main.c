#include "validara.h"

#include <stdio.h>
#include <string.h>

static void usage(const char *prog) {
    printf("Validara — analytical method validation for service offering readiness\n");
    printf("Usage: %s [--help] [--demo] [--dashboard] [--export PATH]\n", prog);
    printf("\nOptions:\n");
    printf("  --demo            Show a sample service-readiness dashboard and report preview\n");
    printf("  --dashboard       Show the default terminal dashboard\n");
    printf("  --export PATH     Export a Markdown method-label report using sample data\n");
    printf("  --help            Show this help text\n");
}

int main(int argc, char **argv) {
    ValidaraProject project;
    validara_load_sample(&project);
    ValidaraAssessment assessment = validara_assess(&project);

    if (argc == 1) {
        validara_print_interactive(stdout, &project, &assessment);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--demo") == 0 || strcmp(argv[i], "--dashboard") == 0) {
            validara_print_dashboard(stdout, &project, &assessment);
            if (strcmp(argv[i], "--demo") == 0) {
                printf("\n--- Method label preview ---\n\n");
                validara_print_report(stdout, &project, &assessment);
            }
            return 0;
        } else if (strcmp(argv[i], "--export") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "--export requires a path\n");
                return 2;
            }
            const char *path = argv[++i];
            if (!validara_export_markdown(path, &project, &assessment)) {
                fprintf(stderr, "Could not write Markdown report to %s\n", path);
                return 1;
            }
            printf("Exported Validara method label to %s\n", path);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }
    return 0;
}
