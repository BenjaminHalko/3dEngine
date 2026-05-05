#include <Engine/Inc/Engine.h>

using namespace Engine;
using namespace Engine::Core;

static int sUniqueId = 0;
class Student
{
  public:
    Student()
    {
        mId = ++sUniqueId;
        mMark = rand() % 100;
        mName = "Ibrahim_" + std::to_string(mId);
    }
    Student(const std::string& name)
        : mName(name)
    {
        mId = ++sUniqueId;
        mMark = rand() % 100;
    }
    Student(const std::string& name, int mark)
        : mName(name)
    {
        mId = ++sUniqueId;
        mMark = mark;
    }

    void Log()
    {
        LOG("Name: %s - ID: %d - Mark: %d", mName.c_str(), mId, mMark);
    }

  private:
    int mId = 0;
    int mMark = 0;
    std::string mName;
};

static int sTeacherId = 0;
class Teacher
{
  public:
    Teacher()
    {
        mId = ++sTeacherId;
        mName = "Teacher_" + std::to_string(mId);
        mSubject = "Subject_" + std::to_string(mId % 5);
    }

    Teacher(const std::string& name, const std::string& subject)
        : mName(name), mSubject(subject)
    {
        mId = ++sTeacherId;
    }

    void Log()
    {
        LOG("Teacher Name: %-20s ID: %3d  Subject: %s",
            mName.c_str(), mId, mSubject.c_str());
    }

  private:
    int mId = 0;
    std::string mName;
    std::string mSubject;
};

static int sCourseId = 0;
class Course
{
  public:
    Course()
    {
        mId = ++sCourseId;
        mName = "Course_" + std::to_string(mId);
        mCredits = (mId % 4) + 1;
    }

    Course(const std::string& name, int credits)
        : mName(name), mCredits(credits)
    {
        mId = ++sCourseId;
    }

    void Log()
    {
        LOG("Course Name: %-20s ID: %3d  Credits: %d",
            mName.c_str(), mId, mCredits);
    }

  private:
    int mId = 0;
    int mCredits = 0;
    std::string mName;
};

int main()
{
    const uint32_t kCapacity = 100;
    const uint32_t kInitial = 70;
    const uint32_t kDeleteCount = 40;

    TypedAllocator<Student> studentPool("StudentPool", kCapacity);
    TypedAllocator<Teacher> teacherPool("TeacherPool", kCapacity);
    TypedAllocator<Course> coursePool("CoursePool", kCapacity);

    std::vector<Student*> students;
    std::vector<Teacher*> teachers;
    std::vector<Course*> courses;

    students.reserve(kCapacity);
    teachers.reserve(kCapacity);
    courses.reserve(kCapacity);

    LOG("Adding 70 items to each pool:");

    for (uint32_t i = 0; i < kInitial; ++i)
    {
        Student* s = (i % 2 == 0)
            ? studentPool.New()
            : studentPool.New("Ibrahim_" + std::to_string(i), rand() % 100);
        s->Log();
        students.push_back(s);

        Teacher* t = (i % 2 == 0)
            ? teacherPool.New()
            : teacherPool.New("Prof_" + std::to_string(i), "Programming");
        t->Log();
        teachers.push_back(t);

        Course* c = (i % 2 == 0)
            ? coursePool.New()
            : coursePool.New("VGP_" + std::to_string(300 + i), (rand() % 4) + 1);
        c->Log();
        courses.push_back(c);
    }

    LOG("Students in pool: %zu", students.size());
    LOG("Teachers in pool: %zu", teachers.size());
    LOG("Courses  in pool: %zu", courses.size());

    LOG("Deleting 40 items from each pool:");

    for (uint32_t i = 0; i < kDeleteCount; ++i)
    {
        studentPool.Delete(students.back());
        students.pop_back();
        teacherPool.Delete(teachers.back());
        teachers.pop_back();
        coursePool.Delete(courses.back());
        courses.pop_back();
    }

    LOG("Students remaining: %zu", students.size());
    LOG("Teachers remaining: %zu", teachers.size());
    LOG("Courses  remaining: %zu", courses.size());

    LOG("Filling each pool to capacity (100):");

    uint32_t toAdd = kCapacity - static_cast<uint32_t>(students.size());

    for (uint32_t i = 0; i < toAdd; ++i)
    {
        Student* s = studentPool.New("TopUp_Student_" + std::to_string(i), rand() % 100);
        s->Log();
        students.push_back(s);

        Teacher* t = teacherPool.New("TopUp_Prof_" + std::to_string(i), "Physics");
        t->Log();
        teachers.push_back(t);

        Course* c = coursePool.New("TopUp_Course_" + std::to_string(i), (rand() % 4) + 1);
        c->Log();
        courses.push_back(c);
    }

    LOG("Students in pool: %zu / %u", students.size(), kCapacity);
    LOG("Teachers in pool: %zu / %u", teachers.size(), kCapacity);
    LOG("Courses  in pool: %zu / %u", courses.size(), kCapacity);

    LOG("Cleaning up all pools:");

    for (Student* s : students) { s->Log(); studentPool.Delete(s); }
    for (Teacher* t : teachers) { t->Log(); teacherPool.Delete(t); }
    for (Course* c : courses) { c->Log(); coursePool.Delete(c); }

    students.clear();
    teachers.clear();
    courses.clear();

    LOG("All pools cleaned up. Students: %zu  Teachers: %zu  Courses: %zu",
        students.size(), teachers.size(), courses.size());

    return 0;
}
