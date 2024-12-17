#ifndef GITHUB_H
#define GITHUB_H

char *github_fetch_image_list(const char *owner, const char *repo,
							  const char *path, const char *ref,
							  int recursive, int page, int per_page);

#endif // GITHUB_H
