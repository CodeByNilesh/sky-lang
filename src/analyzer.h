/* analyzer.h — Type checker header */
#ifndef SKY_ANALYZER_H
#define SKY_ANALYZER_H

#include <stdbool.h>
#include "ast.h"

typedef struct {
    const char *filename;
    int         error_count;
    bool        had_error;
} SkyAnalyzer;

void sky_analyzer_init(SkyAnalyzer *analyzer, const char *filename);
bool sky_analyzer_analyze(SkyAnalyzer *analyzer, SkyASTNode *ast);

#endif
