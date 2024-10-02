let status_updater;
function update_config()
{
    $.ajax({
        type: 'GET',
        url: "api/v1/update_config",
        data:
            {
                admin_password: admin_password,
                //monitor_server: document.getElementById("monitor-server").value,
                monitor_token: document.getElementById("monitor-token").value,
                monitor_types: document.getElementById("monitor-types").value,
                monitor_tasks: document.getElementById("monitor-tasks").value,
                monitor_interval_in_ms: document.getElementById("monitor-interval-in-ms").value,

                notification_smtp_server: document.getElementById("notification-smtp-server").value,
                notification_smtp_username: document.getElementById("notification-smtp-username").value,
                notification_smtp_password: document.getElementById("notification-smtp-password").value,
                notification_smtp_sender_email: document.getElementById("notification-smtp-sender-email").value,
                notification_smtp_receiver_emails: document.getElementById("notification-smtp-receiver-emails").value,

                new_admin_password: document.getElementById("admin-password").value
            },
        success: function (result) {
            if (result["status"] === "success") {
                admin_password = document.getElementById("admin-password").value;
                mdui.snackbar(result["message"]);
            }
            else
                mdui.snackbar(result["message"]);
        },
    });
}

function auth() {
    mdui.prompt("Administrator password", "Authentication",
        function (value) {
            $.ajax({
                type: 'GET',
                url: "api/v1/login",
                data:
                    {
                        admin_password: value
                    },
                success: function (result) {
                    if (result["status"] === "success") {
                        admin_password = value;
                        $.ajax({
                            type: 'GET',
                            url: "api/v1/get_config",
                            data:
                                {
                                    admin_password: admin_password
                                },
                            success: function (result) {
                                if (result["status"] === "success") {
                                    //$("#monitor-server").val(result["config"]["monitor"]["server"]);
                                    $("#monitor-token").val(result["config"]["monitor"]["token"]);
                                    $("#monitor-types").val(result["config"]["monitor"]["types"]);
                                    $("#monitor-tasks").val(result["config"]["monitor"]["tasks"]);
                                    $("#monitor-interval-in-ms").val(result["config"]["monitor"]["interval_in_ms"]);

                                    $("#notification-smtp-server").val(result["config"]["notification"]["smtp"]["server"]);
                                    $("#notification-smtp-username").val(result["config"]["notification"]["smtp"]["username"]);
                                    $("#notification-smtp-password").val(result["config"]["notification"]["smtp"]["password"]);
                                    $("#notification-smtp-sender-email").val(result["config"]["notification"]["smtp"]["sender_email"]);
                                    $("#notification-smtp-receiver-emails").val(result["config"]["notification"]["smtp"]["receiver_emails"]);

                                    $("#admin-password").val(result["config"]["server"]["admin_password"]);
                                    mdui.updateTextFields();
                                } else {
                                    mdui.snackbar(result["message"]);
                                }
                            }
                        });

                        status_updater = window.setInterval(function () {
                                $.ajax({
                                    type: 'GET',
                                    url: "api/v1/update",
                                    data:
                                        {
                                            admin_password: admin_password
                                        },
                                    success: function (result) {
                                        if (result["status"] === "success") {
                                            mdui.snackbar("TODO");
                                        } else {
                                            mdui.snackbar(result["message"]);
                                        }
                                    }
                                });
                            }, 2000);
                        $("#loading").remove();
                        $("#serverinfo-data").removeClass("mdui-hidden");
                    } else {
                        mdui.snackbar(result["message"]);
                        auth();
                    }
                },
            });
        }, function () {
            mdui.snackbar("Permission denied");
            auth();
        },
        {
            history: false,
            modal: true,
            closeOnEsc: false,
            confirmOnEnter: true,
        });
}

window.onload = auth;