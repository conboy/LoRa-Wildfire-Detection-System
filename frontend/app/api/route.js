import { createClient } from '@supabase/supabase-js';

// Create a single supabase client for interacting with your database
const supabase = createClient(process.env.NEXT_PUBLIC_SUPABASE_URL, process.env.NEXT_PUBLIC_SUPABASE_ANON_KEY);

export async function GET() {
    return new Response('Hello, Next.js!', { status: 200 })
}

export async function POST(request) {
    const data = await request.json();
    console.log(data.nodes);
    const { response, error } = await supabase
        .from('nodes')
        .upsert(data.nodes)

    if (error) {
        console.error('Error upserting data:', error);
        return new Response(JSON.stringify({ error: error.message }), { status: 500 });
    } else {
        return new Response(JSON.stringify({ message: "Upsert successful" }), { status: 200 });
    }
}

