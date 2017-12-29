#! /bin/bash

cat kanji_single_line.txt | sed 's/ /\n/g' | awk 'BEGIN{x = 0}{print $0, x % 64, int(x / 64); x++}' > kanji_locations.txt

#cat kanji_single_line.txt | sed 's/ /\n/g' | awk 'BEGIN{x = 0; printf("    ")}{printf("%d, ", x % 64); x++; if (x % 64 == 0){printf("\n    ")}}' > kanji_x.txt
#cat kanji_single_line.txt | sed 's/ /\n/g' | awk 'BEGIN{x = 0; printf("    ");}{printf("%d, ", int(x / 64)); x++; if (x % 64 == 0){printf("\n    ")}}' > kanji_y.txt

echo 'const char *font_char_[] = {' > font.h
cat kanji_single_line.txt | sed 's/ /\n/g' | awk 'BEGIN{x = 0; printf("    ");}{printf("\"%s\", ", $0); x++; if (x % 64 == 0){printf("\n    ")}}' >> font.h
echo '' >> font.h
echo '};' >> font.h
echo '' >> font.h
echo 'int font_x_[] = {' >> font.h
cat kanji_single_line.txt | sed 's/ /\n/g' | awk 'BEGIN{x = 0; printf("    ")}{printf("%d, ", x % 64); x++; if (x % 64 == 0){printf("\n    ")}}' >> font.h
echo '' >> font.h
echo '};' >> font.h
echo '' >> font.h
echo 'int font_y_[] = {' >> font.h
cat kanji_single_line.txt | sed 's/ /\n/g' | awk 'BEGIN{x = 0; printf("    ");}{printf("%d, ", int(x / 64)); x++; if (x % 64 == 0){printf("\n    ")}}' >> font.h
echo '' >> font.h
echo '};' >> font.h
echo '' >> font.h

cat kanji_single_line.txt | sed 's/ /\n/g' | awk 'BEGIN{x = 0}{printf("%s", $0); x++; if (x % 64 == 0){printf("\n")}}' > kanji_matrix.txt