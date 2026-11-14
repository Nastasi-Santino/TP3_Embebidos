#ifndef VERSION_H_
#define VERSION_H_

typedef enum
{
    VERSION_1,
    VERSION_2,
} version_t;

version_t read_version(void);

#endif /* VERSION_H_ */
