struct plat_id_t {
		int num;
		hyper int platform;
		int num_platforms;
		int result;
};

struct plat_info_t {
		hyper int platform;
		int param_name;
		int param_value_size;
		opaque param_value<>;
		hyper int param_value_size_ret;
		int result;
};

struct get_devs_t {
		hyper int platform;
		int device_type;
		int num_entries;
		hyper int devices;
		hyper int num_devices;
		int result;
};

struct create_ctx_t {
		int properties;
		int num_devices;
		hyper int devices;
		hyper int callback;
		hyper int user_data;
		hyper int errorcode_ret;
		hyper int context;
};

struct create_cqueue_t {
		hyper int context;
		hyper int device;
		int properties;
		hyper int errorcode_ret;
		hyper int queue;
};

struct create_prog_ws_t {
		hyper int context;
		int count;
		string strings<>;
		hyper int lengths;
		hyper int errorcode_ret;
		hyper int prog;
};

struct build_prog_t {
		hyper int prog;
		int num_devices;
		hyper int device_list;
		hyper int options;
		hyper int callback;
		hyper int user_data;
		int result;
};

struct create_kern_t {
		hyper int prog;
		string kernel_name<>;
		hyper int errorcode_ret;
		hyper int kernel;
};

struct create_buf_t {
		hyper int context;
		int flags;
		int size;
		opaque host_ptr<>;
		hyper int errorcode_ret;
		hyper int buffer;
};

struct set_kern_arg_t {
		hyper int kernel;
		int arg_index;
		opaque arg<>;
		int result;
};

struct enq_ndr_kern_t {
		hyper int command_queue;
		hyper int kernel;
		int work_dim;
		int global_work_offset;
		int global_work_size;
		int local_work_size;
		int num_events_in_wait_list;
		hyper int event_wait_list;
		hyper int event;
		int result;
};

struct enq_read_buf_t {
		hyper int command_queue;
		hyper int buffer;
		int blocking_read;
		int offset;
		int cb;
		opaque ptr<>;
		hyper int num_events_in_wait_list;
		hyper int event_wait_list;
		hyper int event;
		int result;
};

struct enq_write_buf_t {
		hyper int command_queue;
		hyper int buffer;
		int blocking_write;
		int offset;
		int cb;
		opaque ptr<>;
		hyper int num_events_in_wait_list;
		hyper int event_wait_list;
		hyper int event;
		int result;
}; 

struct finish_t {
		hyper int command_queue;
		int result;
};

struct enq_map_buf_t {
		hyper int command_queue;
		hyper int buffer;
		int blocking_map;
		int map_flags;
		int offset;
		int cb;
		int num_events_in_wait_list;
		hyper int event_wait_list;
		hyper int event;
		hyper int errorcode_ret;
		opaque buf<>;
};

struct release_mem_t {
		hyper int buffer;
		int result;
};

struct release_prog_t {
		hyper int prog;
		int result;
};

struct release_kern_t {
		hyper int kernel;
		int result;
};

struct release_cqueue_t {
		hyper int command_queue;
		int result;
};

struct release_ctx_t {
		hyper int context;
		int result;
};

program OCL_PROG {
		version OCL_VERS {
				plat_id_t GET_PLAT_ID(plat_id_t) = 1;
				plat_info_t GET_PLAT_INFO(plat_info_t) = 2;
				get_devs_t GET_DEVS_ID(get_devs_t) = 3;
				create_ctx_t CREATE_CTX(create_ctx_t) = 4;
				create_cqueue_t CREATE_CQUEUE(create_cqueue_t) = 5;
				create_prog_ws_t CREATE_PROG_WS(create_prog_ws_t) = 6;
				build_prog_t BUILD_PROG(build_prog_t) = 7;
				create_kern_t CREATE_KERN(create_kern_t) = 8;
				create_buf_t CREATE_BUF(create_buf_t) = 9;
				set_kern_arg_t SET_KERN_ARG(set_kern_arg_t) = 10;
				enq_ndr_kern_t ENQ_NDR_KERN(enq_ndr_kern_t) = 11;
				finish_t FINISH(finish_t) = 12;
				enq_map_buf_t ENQ_MAP_BUF(enq_map_buf_t) = 13;
				enq_read_buf_t ENQ_READ_BUF(enq_read_buf_t) = 14;
				enq_write_buf_t ENQ_WRITE_BUF(enq_write_buf_t) = 15;
				release_mem_t RELEASE_MEM(release_mem_t) = 16;
				release_prog_t RELEASE_PROG(release_prog_t) = 17;
				release_kern_t RELEASE_KERN(release_kern_t) = 18;
				release_cqueue_t RELEASE_CQUEUE(release_cqueue_t) = 19;
				release_ctx_t RELEASE_CTX(release_ctx_t) = 20;
		} = 1;
} = 0x23451111;
