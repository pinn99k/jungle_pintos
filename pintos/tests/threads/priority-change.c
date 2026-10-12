/* Verifies that lowering a thread's priority so that it is no
   longer the highest-priority thread in the system causes it to
   yield immediately. */


#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/thread.h"

static thread_func changing_thread;

void
test_priority_change (void) 
{
  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  msg ("Creating a high-priority thread 2.");
  thread_create ("thread 2", PRI_DEFAULT + 1, changing_thread, NULL); //1. 우선순위의 값 32을 지닌 스레드 2를 생성 + changing_thread 함수가 그 스레드가 됨
  msg ("Thread 2 should have just lowered its priority.");  //4. 우선순위값이 기본(31)이니까 제일 큰 우선순위니까 cpu가 이 스레드로 받아 msg호출 
  thread_set_priority (PRI_DEFAULT - 2);  //5. 나는 더 낮아질게~ [31 - 2 = 29] >  {test_pri....() < changin...()} 성립하기에, changing 함수로 스레드 교체
  msg ("Thread 2 should have just exited.");   //7. 우선순위가 제일 큰 스레드가 끝났으니, 다음으로 받아옴
}

static void
changing_thread (void *aux UNUSED) 
{
  msg ("Thread 2 now lowering priority.");  //2. 32의 우선순위를 가진 스레드(우선순위가 제일 큼)로 들어와서 msg 씀
  thread_set_priority (PRI_DEFAULT - 1);   //3. 이 함수 왈 : 나 우선순위 기본우선순위값(31)-1 한 값으로 할게~  그래서 우선순위는 30
  msg ("Thread 2 exiting.");    //6. 우선순위가 제일 커져서, msg 호출 후 코드가 끝
}

/*문제 상황 :현재 상황은 우선순위의 구현이 되어있지 않기에, 그냥 main함수의 msg 3줄만 호출되는 것
  회생방안 - 해결방법 : 우선순위의 구현을 진행 
  회생방안 - 고쳐야하는것 : thread_create(), changing_thread() 여기 두개를 고쳐야함
  yield를 어떻게 써야할지 고려
  현재 상황은 큰 우선순위 값을 기준으로 뽑는거니까, max로 
  
  
  */